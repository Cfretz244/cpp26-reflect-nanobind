dedup_key: binder-name-of-typedef-anonymous-struct
layer: BINDER

# Binder derives class name via `identifier_of(^^T)`, which is ill-formed for C-style `typedef struct {...} name;` types

## Smallest trigger

A C-style anonymous struct named only through a typedef, bound via `nb::reflect_`:

```cpp
typedef struct { int x; int y; } point_t;   // the tinyobjloader idiom
nb::reflect_<^^point_t>(m);                  // Gate 4 compile error
```

Every public type in tinyobjloader v1.0.7 uses this idiom (`attrib_t`, `shape_t`,
`mesh_t`, `material_t`, `index_t`, `tag_t`, `path_t`, `texture_option_t`, ...), so the
whole run fails to compile -- there is no smaller bindable subset.

## First diagnostics

```
nb_reflect.h:2252:20: error: constexpr variable 'name' must be initialized by a constant expression
  ... in instantiation of 'reflect_class<tinyobj::material_t, ...>' requested here
  ... in call to 'entity_name<^^(type)>()'        (nb_reflect.h:2252)
  ... in call to 'identifier_of(^^(type))'        (nb_reflect.h:194)
meta:853:10: note: subexpression not valid in a constant expression
nb_reflect.h:194:39: note: reflected a type is anonymous and has no associated identifier
```

## Root cause

`reflect_class<T, Rs...>` (nb_reflect.h:2243) computes the Python class name with
`entity_name<^^T>()`, whose non-template branch is
`std::define_static_string(std::meta::identifier_of(R))` (nb_reflect.h:188-195).

For a `typedef struct {...} name;` type, the typedef *name* reflection (e.g.
`^^tinyobj::material_t`) does have an identifier -- `has_identifier` is `true` and
`identifier_of` yields `"material_t"`. But `reflect_class` is parameterized on the
resolved *type* `T`, and `^^T` (formed inside the template, after argument substitution)
reflects the underlying **anonymous record**, not the typedef. For that record
`std::meta::has_identifier(^^T)` is **false** and `identifier_of(^^T)` is not a constant
expression -- so the `constexpr auto name = ...` initialization is ill-formed.

Confirmed in isolation:

```cpp
typedef struct { int x; } point_t;
template <typename T> consteval bool h() { return std::meta::has_identifier(^^T); }
static_assert(h<point_t>() == false);   // the typedef name is lost at substitution
```

The typedef name IS recoverable: `std::meta::display_string_of(^^point_t)` returns
`"point_t"` even through the resolved type, and the original typedef-name reflection
passed into the `reflect_` pack (`^^tinyobj::material_t`) still answers
`identifier_of`. The binder discards both by routing the name through `^^T`.

## Suggested fix direction (not applied -- binder is off-limits in this run)

`entity_name` should fall back when `has_identifier(^^T)` is false: either consult
`display_string_of` (which yields the typedef spelling for these types), or thread the
ORIGINAL reflection (the one in the `Rs...` pack / the member's `type_of`, which retains
the typedef) into `reflect_class` instead of deriving the name from the resolved `T`.
The same `identifier_of(^^T)` path is used for transitively-discovered member types
(`tag_t`/`path_t`/`texture_option_t` here), so the fallback must cover the discovery walk
too, not just the explicitly-listed pack entries.

## Minimal standalone repro

`findings_draft/repro_anon_typedef_struct.cpp` -- mirrors `entity_name`/`reflect_class`'s
name derivation in ~15 lines; fails with the identical
"reflected a type is anonymous and has no associated identifier" note.

Compile:
```
clang++ -std=c++26 -freflection-latest -fentity-proxy-reflection -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem <TC>/include/c++/v1 \
  -fsyntax-only repro_anon_typedef_struct.cpp
```

## Notes on classification

This is a BINDER limitation, not a toolchain bug: `identifier_of` correctly reports that
an anonymous record has no identifier (it is anonymous); the diagnostic is accurate. The
binder simply asks the wrong reflection for the name. No compiler crash/ICE -- a clean,
accurate constant-expression rejection.
