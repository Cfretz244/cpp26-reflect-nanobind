# FILED — bloomberg/clang-p2996 issue + PR (2026-06-11)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/322
- **PR:** https://github.com/bloomberg/clang-p2996/pull/324 — branch
  `reflect-param-name-instantiation-state` on `Cfretz244/llvm-project`
  (commit `0b93479`: `a015a30` cherry-picked onto their `p2996` tip
  `837da39`; commit message free of internal finding tags).
- **Tracking:** listed on https://github.com/bloomberg/clang-p2996/issues/308
  under category **B** (wrong-source-of-truth metafunction answers), with a
  note that it is the redeclaration-chain sibling of the sugar-blind family.

## Validation evidence behind the filing (all on this laptop)

- PR-state compiler built from `reflect-validation-tc18-20`
  (= base `837da39` + the TC-0018 fix + this fix, Release+assertions,
  AArch64) in `toolchain-build`:
  - `repros/TC-0020/repro.cpp` compiles in BOTH variants (with and without
    `-DFORCE_DEFINITION`) — the post-fix deterministic behavior
    (inconsistently-named parameter has no identifier; consistently-named
    control keeps its name);
  - regression test `param-name-consistency-instantiation.pass.cpp`
    compiles AND runs (exit 0);
  - `clang/test/Reflection`: parity with base.
- Pre-fix flip: directly observed on the local branch before the fix —
  the explicit-instantiation variant answered "val", the other "value"
  (the minimization that isolated in-place parameter replacement during
  member-definition instantiation as the mechanism).
- Full local toolchain (reflection-p2996 with the fix): binder suite
  131/131; full 36-run corpus sweep green, and **eigen's surface_diff_ignore
  is removed** — the corpus's only Gate 6b mismatch is gone at the root.

## Found-via provenance

Eigen's three-way corpus run: the constexpr binding TU (binding lambdas
odr-use setConstant, instantiating its definition before query time) and
the emit generator TU (reflection only) rendered different Python keyword
names for the same method — caught by the module-vs-module surface diff,
not by any compiler diagnostic.
