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

NOT json-specific: `unordered_dense` (the next-heaviest text-rendering
fixpoint; its clang emit stage 1 takes 116 s under a
`-fconstexpr-steps=1000000000` budget and finishes comfortably) hits the
same OOM under g++. The correlation is with heavy consteval STRING
RENDERING (the emit generator), not reflection walking per se — both runs'
constexpr lanes, with the same raised budgets, compile fine under g++. If
another run hits this wall, record it against this dedup_key and disable
that run's gcc16 emit lane the same way. With two runs affected, the emit
backend's chunked-evaluation design (emit_item_chunk_v) likely needs a
GCC-specific chunking strategy — a binder change, tracked separately.

## Mitigation status

- `runs/json/meta.toml` `[gcc16] emit_enabled = false` (constexpr lane green:
  `constexpr=E`, 37 s).
- `runs/unordered_dense/meta.toml` `[gcc16] emit_enabled = false` (constexpr
  lane green after the GCC-0008 fix).
- Possible future binder-side mitigation: render per-class chunks in even
  smaller independent constant evaluations under GCC.
