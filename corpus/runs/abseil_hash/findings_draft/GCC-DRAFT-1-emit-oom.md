# GCC-DRAFT-1 — abseil_hash emit generator OOM-killed (GCC-0007 wall)

- dedup_key: `gcc16-constexpr-memory-emit-generator` (same as corpus/findings/GCC-0007)
- run: `abseil_hash` (tier 6, Abseil hash containers)
- compiler: g++ (GCC) 16.1.0 aarch64-linux-gnu (`gcc16-reflect` container)

## Symptom

The gcc16 emit-lane GENERATOR (`binding/gen_emit.cpp`,
`nb::write_bindings<flat_hash_map<int,string>, flat_hash_set<int>,
node_hash_map<int,string>, hmtest>`) exhausts the container's ~31 GB and
cc1plus is OOM-killed:

```
g++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
BUILD_FAIL_STAGE=emit_gen_compile
```

No error message, no step-limit diagnostic — the bare "Killed" signature of
the GCC-0007 memory wall. The generator ran ~827 s before death.

## Why this is the genuine wall, not the unordered_dense false alarm

- The **constexpr lane is fully green**: `constexpr=E`, 30 s compile, 7/7
  differential tests pass — including `test_no_policy_explosion`, which asserts
  none of the container_internal POLICY types (FlatHashMapPolicy, etc.) leaked
  into the bind set. So this is NOT the `__gnu_cxx`/libstdc++-leakage bind-set
  bloat that originally made unordered_dense look affected.
- clang-p2996 renders the SAME generator fine: emit lane green, 240 s, 509-line
  / 238 KB generated TU (see `result.json`).
- abseil_hash's 509-line TU is LARGER than unordered_dense's passing 199-line
  TU (which already peaked at ~28.5 GiB of 31 GiB). The three hash-container
  surfaces + raw_hash_map/raw_hash_set ancestry + the heterogeneous query
  surface rendered as qualified member-template calls push the consteval
  string-rendering workload past the wall.

## Resolution (sanctioned per AGENT_PROMPT_GCC16 item 12)

- `runs/abseil_hash/meta.toml`: added `[gcc16] emit_enabled = false` with a
  comment citing GCC-0007.
- Run added to GCC-0007's Scope + Mitigation lists
  (corpus/findings/GCC-0007-emit-generator-constexpr-memory.md): json and
  abseil_hash are now the two runs disabled outright; unordered_dense remains
  the passing-but-thin third datapoint.
- No binder edit, no GCC repro added: this is the already-catalogued
  performance wall (GCC-0007 is the umbrella repro/finding), not a new bug
  class. A future binder-side mitigation (per-class chunked evaluation under
  GCC) would re-enable these emit lanes.

## Result

`result-gcc16.json`: outcome **E**, constexpr lane E (7/7, 27.7 s), emit lane
dropped (disabled), surface_diff status `skip` (no emit lane to diff).
