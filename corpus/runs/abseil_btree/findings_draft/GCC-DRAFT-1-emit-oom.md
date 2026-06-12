# GCC-DRAFT-1 — abseil_btree emit generator hits the GCC-0007 OOM wall

- dedup_key: `gcc16-constexpr-memory-emit-generator`  (same as finding GCC-0007)
- run: `abseil_btree`
- compiler: g++ (GCC) 16.1.0 aarch64-linux-gnu (`gcc16-reflect` container, 31 GiB)
- status: sanctioned emit-lane disable (protocol item 12). NOT a new finding —
  this run is a new instance of the existing GCC-0007 wall.

## Symptom

The gcc16 emit GENERATOR for abseil_btree (`nb::write_bindings<...>` rendering
the full btree-container binding TU as text inside constant evaluation)
exhausts the container's memory and cc1plus is OOM-killed:

```
g++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
BUILD_FAIL_STAGE=emit_gen_compile
```

No error diagnostic — the bare "Killed signal" signature of GCC-0007. Death at
~196 s of stage-1 compile.

## Root cause

Identical to GCC-0007: heavy consteval STRING RENDERING in the emit generator.
abseil_btree is the ordered sibling of abseil_hash — it renders the
btree_map_container/btree_set_container/btree_container ancestry plus the
heterogeneous query surface (contains/count/find/erase/at/equal_range/
lower_bound/upper_bound and operator[] → __getitem__) as qualified
member-template calls. The generated TU is 536 lines (larger than
abseil_hash's 509). clang-p2996 renders the same TU in 365 s at ~2 GB-class
RSS; g++ blows past 31 GiB. The same reflection walk binding via splices (the
constexpr lane) compiles fine under g++ in 23.3 s.

This is a resource wall, not a correctness gap: the constexpr lane is fully
green (constexpr=E, 4/4 tests, bind set clean — not the libstdc++-leakage false
alarm that originally mis-flagged unordered_dense).

## Mitigation

`runs/abseil_btree/meta.toml` gains a `[gcc16]` table with `emit_enabled = false`
and a justification comment citing GCC-0007. The constexpr lane stays required
and green. The run's outcome on gcc16 is **E** (constexpr=E; emit lane
backend-disabled; surface diff skipped because emit is off).

Updated `corpus/findings/GCC-0007-emit-generator-constexpr-memory.md` Scope and
Mitigation sections to add abseil_btree as the fourth case / second genuine
container run.

## Files touched

- `runs/abseil_btree/meta.toml` (added `[gcc16] emit_enabled = false`)
- `corpus/findings/GCC-0007-emit-generator-constexpr-memory.md` (Scope +
  Mitigation: abseil_btree added)
