# BINDER-0004 — Infinite recursion + redundant re-walk on self-referential / heavy types

- **Status:** FIXED (binder)
- **Found via:** Phase 1, nlohmann/json (`basic_json`)
- **File:** `nanobind/include/nanobind/nb_reflect.h` — the STL-caster and user-spec collection walks

## Two defects, one root area

The binder's two compile-time "collection" walks — `collect_stl_types` (which `<nanobind/stl/*.h>`
casters a reflected set needs) and `collect_user_specs_from_type` (which user class-template
specializations to bind) — both recurse through a type's template arguments. Bringing up
`nlohmann::json` exposed two problems:

1. **Infinite recursion on self-referential types.** `nlohmann::json` is `basic_json<ObjectType=
   std::map<std::string, basic_json>, ArrayType=std::vector<basic_json>, ...>` — its own template
   arguments contain itself. Both walks deduped the *push* (`info_vec_contains(out, type)`) but
   **not the recursion**, so they descended `basic_json → map → basic_json → …` forever, hitting
   the constexpr step limit ("possible infinite loop?"). Any self-referential type (e.g. a tree
   `Node { std::vector<Node> children; }`) would trigger this.

   **Fix:** add a `visited` set that guards the *recursion* (return early if the type was already
   walked), in addition to the existing out-dedup.

2. **Redundant re-walk → constexpr step explosion.** `collect_class_stl_types` /
   `collect_class_user_specs` called the per-type walk with a **fresh `visited` per member**, so a
   class whose hundreds of members each mention the same heavy type (basic_json) re-walked that
   type's entire graph once *per member* — O(members × graph) constexpr work.

   **Fix:** thread a **single shared `visited`** through the whole class/scope walk
   (`collect_scope_*`, `collect_class_*`, and the `required_*` entry points), so each type's graph
   is walked once total — O(graph).

## Verification

- Existing reflection suite still passes **41/41**; all four prior corpus runs still **E**
  (`_fixture_pod`, `_fixture_virtual`, `linalg`, `glm`).
- For json these fixes cleared the STL walk and cut the user-spec cost substantially, but
  `basic_json` remains too large for the toolchain's constexpr budget and ICEs when pushed past it
  (TC-0002) — so json stays **B** (intractable heavy type). The fixes are valuable independent of
  json: they make the binder correct on self-referential types and much cheaper on large ones.

## Upstream

Binder (nanobind fork) fix, lands on `mk-reflect`.
