# NOTE — unordered_dense should come OFF GCC-0007's scope/mitigation lists

dedup_key: `gcc16-constexpr-memory-emit-generator` (scope correction, not a new finding)

GCC-0007 (emit-generator constant-evaluation memory blow-up) lists
unordered_dense as a second affected run and `[gcc16] emit_enabled = false`
was recorded in this run's meta.toml. That OOM evidence was gathered while
libstdc++'s `__gnu_cxx::__normal_iterator` specializations were leaking into
the bind set (BINDER-DRAFT-1, fixed in `is_in_std`): the extra discovered
specs inflated the emit generator's rendering workload past the container's
~31 GiB.

With the fix, the emit lane PASSES under g++ 16.1: observed peak ~28.5 GiB,
emit lane outcome E, generated TU renders and the module passes the
differential suite + Gate 6b surface diff. meta.toml has been re-enabled
(emit_enabled removed) with a comment warning the margin is thin (~91% of
container memory — run the emit lane solo).

Supervisor actions:
- Remove unordered_dense from GCC-0007's Scope/Mitigation lists (json remains
  the canonical case; its 1.69 GB-vs->31 GB calibration is unaffected).
- The ~20x memory ratio is still real here — unordered_dense is a good
  SECONDARY data point for the eventual upstream performance report
  (fits on clang in ~2 GiB-class usage at 116 s; ~28.5 GiB under g++),
  just not an emit-disable case anymore.
