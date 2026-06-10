dedup_key: binder-no-completeness-gate-for-incomplete-pointer-in-member-signature
layer: BINDER

# Binder binds members whose signature names a pointer to an INCOMPLETE non-template class, hard-erroring in nanobind's caster

## Summary

`reflect_bind_member_function` has no bindability gate for a member whose return type or
parameter type is a pointer (or reference) to a **forward-declared, incomplete, non-template
class type**. The binder routes such a member to `cls.def(...)`, which instantiates nanobind's
`type_caster` / descriptor machinery on the incomplete type. That machinery uses
`typeid(Type)` (`nb_descr.h:32`, `nb_cast.h:467`/`481`) and `std::is_base_of` /
`std::is_polymorphic` (libc++ `__type_traits/...`), all of which are a HARD compile error on an
incomplete type. Outcome B (binding fails to compile) with no diagnostic from the binder
itself -- the error surfaces from deep inside libc++/nanobind.

This is the plain-class analogue of the completeness gate the binder already has for *user
class-template specializations* (BINDER-0014's `is_complete_type` gate +
`type_mentions_excluded`'s "user spec, not std, not complete" branch). That gate only fires
for specializations with template arguments; a non-template forward-declared `struct Foo;`
used as `Foo*` in a member signature is not covered and reaches nanobind.

## Field shape (pugixml v1.15)

pugixml's handle classes expose their pImpl pointer and an explicit-from-pointer ctor over
forward-declared structs:

```cpp
namespace pugi {
    struct xml_node_struct;        // forward-declared, never defined in the public header
    struct xml_attribute_struct;

    class xml_node {
    public:
        explicit xml_node(xml_node_struct* p);     // param: pointer to incomplete type
        xml_node_struct* internal_object() const;  // return: pointer to incomplete type
        // ...
    };
    class xml_attribute { /* same shape over xml_attribute_struct */ };
}
```

Binding `^^pugi::xml_node` / `^^pugi::xml_attribute` head-on fails to compile:

```
nanobind/include/nanobind/nb_descr.h:32:25: error: 'typeid' of incomplete type 'pugi::xml_node_struct'
nanobind/include/nanobind/nb_cast.h:467:29:  error: 'typeid' of incomplete type 'Type' (aka 'pugi::xml_node_struct')
.../__type_traits/is_base_of.h:26:83:        error: incomplete type 'pugi::xml_node_struct' used in type trait expression
.../__type_traits/is_polymorphic.h:26:69:    error: incomplete type 'pugi::xml_node_struct' ...
```

## Smallest trigger

```cpp
// trigger.cpp -- compile as a reflect_ module TU (via corpus/lib/build_module.sh)
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
namespace nb = nanobind;
namespace demo {
    struct impl;                       // forward-declared, never defined
    struct Handle {
        impl* get() const;             // pointer to incomplete type in a bound signature
    };
}
NB_MODULE(demo_ext, m) { nb::reflect_<^^demo::Handle>(m); }
```

VERIFIED minimal: `bash corpus/lib/build_module.sh trigger.cpp demo_ext <out>` =>
```
.../__type_traits/is_base_of.h:26:83: error: incomplete type 'demo::impl' used in type trait expression
nanobind/include/nanobind/nb_descr.h:32:25: error: 'typeid' of incomplete type 'demo::impl'
nanobind/include/nanobind/nb_cast.h:481:39:  error: 'typeid' of incomplete type 'Type' (aka 'demo::impl')
.../__type_traits/is_polymorphic.h:26:69:    error: incomplete type 'demo::impl' ...
```
(`impl` need not even be definable; a true pImpl is opaque by design.)

## Workaround used in this run

`nb::exclude_<^^pugi::xml_node_struct, ^^pugi::xml_attribute_struct>` in the reflect_ pack
makes the structs opaque, so `type_mentions_excluded` (which strips pointers before testing)
skips `internal_object()` and the `xml_*(xml_*_struct*)` ctors. The run then reaches outcome E.
This works but requires the binding author to enumerate every opaque internal struct by hand.

## Suggested direction (for the binder maintainer, not done here)

A natural fix mirrors the existing template-spec completeness gate: in `fn_mentions_excluded`
(or a sibling gate consulted on every bind path), also skip a member whose signature mentions
a non-template class type that is forward-declared and `!is_complete_type` in this TU -- a
pointer/reference to an opaque type cannot be cast to/from Python anyway, so skipping it is
strictly safer than the current hard error, and matches how incomplete user specs are already
handled. (TC-0012 made `is_complete_type` see through alias sugar, which this would rely on.)

## Notes

- Not a toolchain bug: the reflection queries all succeed; the failure is purely that the
  binder forwards an unbindable signature to nanobind. Filed at BINDER layer.
- The incompleteness is intrinsic to the pImpl idiom -- it is NOT fixable by including more
  headers (pugixml never defines these structs publicly).
