# BINDER-0022 — same-named enums/classes in sibling scopes silently clobbered one module attribute

- **Status:** FIXED (binder: `parent_qualified_name` + collision check in
  `reflect_class`/`reflect_enum`; `reflect_enum` also gained the is_valid idempotence guard).
- **Found via:** corpus/runs/yamlcpp (wave 2; dedup key
  `binder-enum-name-collision-unqualified-namespace`): `YAML::NodeType::value` and
  `YAML::EmitterStyle::value` both bound as module attr `value`; the second clobbered the
  first SILENTLY — `Node().Type()` returned an enum unreachable by name,
  `isinstance(..., m.value)` False.
- **Fix:** when the plain name is already taken in the module dict at bind time, the entity
  binds as `<Parent>_<name>` (e.g. `collide_b_shade`). First binder keeps the plain name.
- **Verification:** `test47_enum_name_collision_qualifies`; the yamlcpp run can re-admit
  EmitterStyle as a residual.
