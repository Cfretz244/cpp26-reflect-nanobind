dedup_key: binder-enum-name-collision-unqualified-namespace

# Two same-named enums in sibling namespaces collide on one Python module attribute

Layer: BINDER

## Smallest trigger

Two enums with the same unqualified identifier, each in its own (sibling)
namespace, both passed to `reflect_`:

```cpp
namespace lib {
  namespace A { enum value { X, Y, Z }; }       // YAML::NodeType::value (Undefined/Null/Scalar/Sequence/Map)
  namespace B { enum value { P, Q, R }; }        // YAML::EmitterStyle::value (Default/Block/Flow)
}
NB_MODULE(ext, m) { nb::reflect_<^^lib::A::value, ^^lib::B::value>(m); }
```

After import, the module has exactly ONE attribute `value`, holding whichever
enum was registered last (`lib::B::value`). The first enum's enumerators
(`X/Y/Z`) are unreachable from Python by name.

In yaml-cpp 0.9.0 this is `YAML::NodeType::value` (Undefined/Null/Scalar/
Sequence/Map -- the return type of the central `Node::Type()`) and
`YAML::EmitterStyle::value` (Default/Block/Flow, the `Style()` return). After
binding both, `m.value` is the EmitterStyle enum; `Node().Type()` returns a
distinct, MODULE-UNNAMED NodeType enum object, so:

```python
m.value.__members__          # {'Default':..,'Block':..,'Flow':..}  -- EmitterStyle won
m.Node().Type()              # repr shows 'value.Null' (correct VALUE)
isinstance(m.Node().Type(), m.value)   # False -- different enum type
m.value.Null                 # AttributeError -- NodeType members not on the module
```

## Root cause

`reflect_enum` (`nb_reflect.h:2503`) names the Python enum with
`reached_entity_name<E, Named>()` -- the UNQUALIFIED identifier
(`identifier_of`), `value` for both. `enum_<E>(m, "value", ...)` registers each
under the bare name on the module; the second `setattr(m, "value", ...)`
clobbers the first. There is no namespace-qualification or collision
disambiguation for enum names (the class path has the same exposure, but enum
names are far more often generic -- `value`, `type`, `kind`, `mode` -- and the
old C++03 "enum-in-a-namespace as a scoped-enum substitute" idiom that yaml-cpp,
Abseil, and many others use makes `Ns::value` ubiquitous).

## First diagnostics

No compile/import error -- silent. Surfaces only behaviorally: the bound
`Type()` value cannot be compared against any module-level enum member, and the
clobbered enum's members are simply absent.

## Suggested direction (not applied)

A reflected enum (and class) whose unqualified name collides with an
already-registered module attribute should be disambiguated (e.g. a
namespace-qualified Python name `NodeType_value` / `EmitterStyle_value`, mirroring
the CamelCase spec-naming the binder already does for template specializations),
or at minimum the binder should not silently let the second registration clobber
the first.

## Run workaround

This run keeps NodeType::value head-on (it is the differential's primary enum --
`Node::Type()`) and drops `EmitterStyle::value` from the reflect_ pack so `m.value`
is unambiguously NodeType (Undefined/Null/Scalar/Sequence/Map). EmitterStyle is
recorded in skipped_features. No test was weakened: the differential asserts on
`Node::Type()` enum identity and members directly.
