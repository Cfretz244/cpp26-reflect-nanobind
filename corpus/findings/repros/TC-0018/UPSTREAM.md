# FILED — bloomberg/clang-p2996 issue + PR (2026-06-11)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/321
- **PR:** https://github.com/bloomberg/clang-p2996/pull/323 — branch
  `reflect-large-pack-arg-counts` on `Cfretz244/llvm-project` (commit
  `87d836f`: `4e0d19f` cherry-picked onto their `p2996` tip `837da39`;
  commit message free of internal finding tags; code comments clean).
- **Tracking:** listed on https://github.com/bloomberg/clang-p2996/issues/308
  under category **E (constant-evaluator robustness)** (tick when merged).

## Validation evidence behind the filing (all on this laptop)

- PR-state compiler built from `reflect-validation-tc18-20`
  (= base `837da39` + this fix + the TC-0020 fix, Release+assertions,
  AArch64) in `toolchain-build`:
  - `repros/TC-0018/repro.cpp` (define_static_string at 32768) compiles;
  - the plain-C++ no-reflection probe (32768-element char pack via
    `make_integer_sequence`) compiles;
  - regression test `define-static-string-large.pass.cpp` compiles AND
    runs (32767 / 32768 / 50000 round-trips, exit 0);
  - `clang/test/Reflection`: parity with base (see lit output in the
    session log; the pre-existing `splice-exprs.cpp` failure only).
- Failure at base: directly observed pre-fix on the local branch (identical
  code at these sites to pristine base) — both repros fail with
  "excess elements in array initializer"; the first fix attempt (PackIndex
  fields only) demonstrably did NOT cure it, isolating
  `SubstNonTypeTemplateParmPackExpr::NumArguments:15` as the root cause.
- Full local toolchain (reflection-p2996 with the fix): binder suite
  131/131; full 36-run three-way corpus sweep green.

## Found-via provenance

The emit backend's first full-fixture generated TU (~50KB) failed to lift
into static storage via define_static_string; minimized to a two-line
length probe, then to the plain-C++ pack shape (which should reproduce in
upstream llvm/llvm-project — noted in the PR).
