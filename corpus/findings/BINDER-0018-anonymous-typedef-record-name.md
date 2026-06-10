# BINDER-0018 — `typedef struct {...} name_t;` types hard-errored the name computation

- **Status:** FIXED (binder, `nb_reflect.h`: `reached_entity_name` + anonymous fallback in `entity_name`)
- **Found via:** corpus/runs/tinyobjloader (wave 1, THE B-blocker; dedup key
  `binder-name-of-typedef-anonymous-struct`). Every public tinyobjloader type uses the C idiom —
  no bindable subset survived.
- **Symptom:** `nb::reflect_<^^point_t>` failed to compile: `constexpr variable 'name' must be
  initialized by a constant expression` — `identifier_of(^^T)` is ill-formed because `^^T`, formed
  after substitution, reflects the ANONYMOUS record (`has_identifier == false`), not the typedef.
- **Root cause(s):** (1) `entity_name<^^T>` had no fallback for identifier-less types; (2) the
  typedef name is NOT recoverable from the record itself — `display_string_of(record)` is just
  `"(anonymous type)"`; the name lives only on the alias declaration the walk reached the type
  through, which `reflect_class<T>` discarded.
- **Fix:** `reflect_class`/`reflect_enum` now take a `Named` reflection — the namespace member or
  explicit `reflect_` argument the type was REACHED through (the alias for typedef'd anon records;
  `^^void` on the base-recursion and spec paths). `reached_entity_name<T, Named>` prefers the
  resolved type's own name, falls back to the alias identifier, and returns nullptr for a truly
  anonymous type — which both binders treat as a graceful skip instead of a hard error.
- **Verification:** `test44_anonymous_typedef_names` (point_t fields, color_t enum values);
  tinyobjloader re-gated **B→E** (attrib_t/shape_t/mesh_t/index_t/material_t head-on, 6 tests).
