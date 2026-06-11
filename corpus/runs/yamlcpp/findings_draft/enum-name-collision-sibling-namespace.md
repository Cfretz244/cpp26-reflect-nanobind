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

## Resolution (BINDER-0022 -- landed in the binder)

`reflect_enum` and `reflect_class` now consult the module dict at bind time and,
when the plain unqualified name is already taken, register the entity under a
parent-qualified Python name `"<Parent>_<name>"` instead of silently clobbering
(`parent_qualified_name<R>(name)` + a runtime `hasattr(m, name) ? qual : name`
guard; `reflect_enum` also gained the `is_valid` idempotence guard so a re-reached
enum does not re-register / trip the collision path). The EMIT backend renders the
identical runtime `nb::hasattr(...) ? qual : name` line (nb_reflect_emit.h's
`emit_enum_def_text` + class twin), so both modules expose the same pair of names.

For this run: NodeType::value is first in the reflect_ pack -> keeps the bare
module attribute `value` (the differential's primary enum, `Node::Type()`);
EmitterStyle::value is second -> binds parent-qualified as `EmitterStyle_value`.
test_bindings.py::test_emitterstyle_collision_parent_qualified asserts both names
and the distinct enum identities against the native oracle's EmitterStyle ground
truth; Gate 6b's surface diff cross-checks the pair name-for-name across the two
backends. No test was weakened; EmitterStyle is no longer in skipped_features.
