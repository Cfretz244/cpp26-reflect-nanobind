# BINDER-0008 — Abseil hash/btree containers & FixedArray: internal-type explosion + template-only APIs

- **Status:** CLOSED (items 1 + 2; item 3 is an Abseil-side residual). Landed as two binder
  changes during the Abseil buildout:
  1. **Reachability-based discovery + base flattening** — a type binds only when reachable
     (seed or public-member-signature); bases and a spec's own template args never qualify.
     The ~35-type `container_internal` explosion collapsed to the handful of
     signature-reachable internals; unbound facade ancestry flattens onto the container.
  2. **Member function templates via default substitution** — the heterogeneous query APIs
     (`template <class K = key_type> contains/find/count/erase/at/operator[]`) bind via
     their default instantiation (with the TC-0004 dispatch-level-substitution workaround).
  Proof: `corpus/runs/abseil_hash` (flat_hash_map/set, node_hash_map) and
  `corpus/runs/abseil_btree` (btree_map/set) at outcome **E** with full query-surface
  differentials incl. `__getitem__`.
  3. **FixedArray residual (still blocked, retested after both changes):** binding
     `absl::FixedArray<int>` instantiates the Abseil-internal `AsValueType` helper, which
     hard-errors (`fixed_array.h:431`: returns `int(*)[0]` where `int*` is expected). An
     Abseil-side fix or source annotation is required; not a binder issue.
- **Found via:** the Abseil expansion (`corpus/runs/abseil_containers`); InlinedVector binds
  cleanly and carries the run to E, the hash/btree maps and FixedArray are excluded.
- **Files:** `nanobind/include/nanobind/nb_reflect.h` (transitive class discovery; member-template
  handling), plus an Abseil-internal toolchain interaction for FixedArray.

A cluster of issues that block a *clean* binding of Abseil's associative/fixed containers. Two
prerequisite binder fixes already landed and are needed here too: **BINDER-0006** (array data
members skipped) and a non-copy-assignable-member → `def_ro` fallback (so a move-only
`node_handle` inside `insert_return_type` no longer breaks `def_rw`). With those, the hash/btree
maps **compile, link, and import** — but are not yet *usable* or *clean*:

1. **Internal-type explosion.** `absl::flat_hash_map<K,V>` is a thin facade over
   `absl::container_internal::raw_hash_map` / `raw_hash_set` (its real base lives in
   `container_internal`). Reflecting it transitively binds ~35 internal types
   (`FlatHashMapPolicy`, `node_handle`, `btree_node`, `common_params`, `map_slot_policy`, …) under
   long mangled Python names. A production binding would not expose these.
   *Fix direction:* skip transitive binding of types whose enclosing namespace is an
   implementation namespace (`*_internal`, `detail`) — but note the public container's own base
   lives in `container_internal`, so the inheritance handling must special-case "bind the base's
   members but don't surface the internal type."

2. **Query/insert APIs are member templates.** `contains`/`count`/`find`/`erase(key)`/`operator[]`
   take a `key_arg<K>` for heterogeneous lookup and are **member function templates**, which the
   binder correctly skips. So the directly-bound surface is only `size`/`empty`/`clear`. Populating
   and querying from Python needs either fixture factories (like the json/fmt runs) or
   member-function-template support (a roadmap item). `emplace`/`try_emplace` are likewise templates.

3. **`absl::FixedArray<int>`** hits an Abseil-internal `AsValueType` instantiation
   (`fixed_array.h:431`: `return std::addressof(ptr->array)` returns `int(*)[0]`, not `int*`) when
   the binder instantiates its storage wrapper — independent of BINDER-0006's array-member skip.
   Needs either not instantiating that internal helper or an Abseil-side `[[=reflect::skip]]`.

## Minor adjacent gaps (same Abseil expansion)

- **`int128::operator int/long/...` collide on `__int__`** (multiple integral conversion operators;
  last-bound wins, picking a narrow one — `int(int128(1000000))` returns `64`). Tests use `str()`.
  *Fix direction:* among integral conversion operators, prefer the widest for `__int__`.
- **Unary free operators are not bound.** `is_bindable_free_operator` requires exactly 2
  parameters, so a unary free `operator-(int128)` is skipped (no `__neg__`).

## Workaround in the corpus

`corpus/runs/abseil_containers` binds `absl::InlinedVector<int,4>`/`<double,2>` (clean, rich) and
excludes the hash/btree maps + FixedArray. The numeric/time/status runs bind their types directly.
dedup_key: `abseil-container-internal-explosion`.
