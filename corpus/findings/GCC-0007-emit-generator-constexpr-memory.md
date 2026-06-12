# GCC-0006 — constant-evaluation memory blow-up at emit-generator scale (~20x clang)

- dedup_key: `gcc16-constexpr-memory-emit-generator`
- compiler: g++ (GCC) 16.1.0 aarch64-linux-gnu (`gcc:16` docker image)
- status: OPEN — no in-compiler workaround found; json's gcc16 emit lane is
  disabled via `[gcc16] emit_enabled = false` with this finding as the
  justification. Upstream report planned (Phase 4; this is a
  performance/scalability report, not an ICE).
- found by: corpus run `json` (the corpus's heaviest emit generator)

## Symptom

Compiling the json run's emit GENERATOR (`gen_emit.cpp`, which renders the
complete ~basic_json binding TU as text inside constant evaluation —
`nb::write_bindings<...>`) exhausts all available memory and cc1plus is
OOM-killed:

```
g++ -std=c++26 -freflection -fconstexpr-ops-limit=400000000 -DJSON_NO_IO ... gen_emit.cpp
g++: fatal error: Killed signal terminated program cc1plus
```

## Calibration (same TU, same flags-equivalent, 2026-06-11)

| compiler | peak RSS | wall | result |
|---|---|---|---|
| clang-p2996 (host, `-fconstexpr-steps=400000000`) | **1.69 GB** | 587 s | succeeds |
| g++ 16.1 (container, `-fconstexpr-ops-limit=400000000`) | **> 31 GB** | n/a | OOM-killed |

An ~20x+ memory ratio on identical source. Not the ops budget (evaluation
dies by memory long before any limit diagnostic), not GC tuning
(`--param ggc-min-expand=10 --param ggc-min-heapsize=131072` made no
difference), and not the constexpr call cache (`-fconstexpr-cache-depth=1`
also OOM-killed). The constexpr lane of the SAME run — the same reflection
walk binding directly via splices instead of rendering text — compiles fine
under g++ (37 s), so the blow-up is specific to string-building-heavy
constant evaluation, presumably the evaluator retaining every intermediate
std::string allocation for the duration of each (large) top-level
evaluation.

## Scope

json is the canonical case. `unordered_dense` initially appeared affected,
but that OOM evidence was gathered while libstdc++'s `__gnu_cxx` iterator
specs were leaking into the bind set (fixed in `is_in_std`, wave 1); with
the fix its emit lane PASSES — at ~28.5 GiB peak vs clang's ~2 GiB-class for
the same 199-line generated TU (116 s). `abseil_hash` is the THIRD case and a
genuine one: its generator renders the three hash-container surfaces
(flat_hash_map/flat_hash_set/node_hash_map, the raw_hash_map/raw_hash_set
ancestry, and the heterogeneous query surface as qualified member-template
calls — a 509-line TU, larger than unordered_dense's passing 199 lines).
cc1plus was OOM-killed after ~827 s with the bare "Killed signal terminated
program cc1plus". This is NOT the libstdc++-leakage false alarm: the run's
constexpr lane is fully green (30 s, 7/7 tests incl. its `test_no_policy_explosion`
structural check, so the bind set is verified clean), and clang-p2996 renders
the same TU in 240 s. So the scope is: TWO runs disabled outright (json,
abseil_hash) and the ~15–20x memory ratio confirmed on a third, passing run
(unordered_dense) — strong material for the upstream performance report. The
correlation is with heavy consteval STRING RENDERING (the emit generator),
not reflection walking per se: all three runs' constexpr lanes, with the same
raised budgets, compile fine under g++. abseil_hash being killed where
unordered_dense survives suggests the wall scales with generated-TU size and
the container/member-template surface is past it.

NOTE: json's >31 GiB evidence was ALSO gathered before the `is_in_std` fix;
re-test before treating its disable as final (supervisor task open). If
another run hits this wall, record it against this dedup_key and disable
that run's gcc16 emit lane the same way; the emit backend's
chunked-evaluation design (emit_item_chunk_v) may need a GCC-specific
chunking strategy — a binder change, tracked separately.

## Mitigation status

- `runs/json/meta.toml` `[gcc16] emit_enabled = false` (constexpr lane green:
  `constexpr=E`, 37 s; pending re-test post-`is_in_std`).
- `runs/abseil_hash/meta.toml` `[gcc16] emit_enabled = false` (constexpr lane
  green: `constexpr=E`, 30 s, 7/7 tests; emit generator OOM-killed at ~827 s,
  509-line TU; bind set verified clean — not the leakage false alarm).
- `runs/unordered_dense`: re-ENABLED after the `is_in_std` fix; passes with
  a thin margin (~91% of the 31 GiB container — run its emit lane solo).
- Possible future binder-side mitigation: render per-class chunks in even
  smaller independent constant evaluations under GCC.
