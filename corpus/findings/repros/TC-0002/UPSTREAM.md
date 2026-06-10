# FILED — bloomberg/clang-p2996 issue + PR (2026-06-09)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/288
- **PR:** https://github.com/bloomberg/clang-p2996/pull/289 — branch
  `reflect-evalctx-uaf` on `Cfretz244/llvm-project` (commit `0d73489`: the SemaExpr.cpp
  part of the original local fix `269d3e3` + the `CheckLValueToRValueConversionOperand`
  hardening from `b82861b` + the regression test, squashed onto their `p2996` tip
  `837da39`).

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here: 64-deep evaluation-time instantiation cascade entirely inside one
  deferred immediate invocation (design notes in its header). The deep nesting MUST be
  driven by evaluation-time metafunction values — anything resolvable at parse time
  instantiates before the held-reference window opens and reproduces nothing.
- Fixed compiler + scratch address probe (`&ExprEvalContexts.back()` compared across
  `HandleImmediateInvocations`): fires exactly once per compile of the repro; silent on
  a plain-consteval negative control.
- Fix-reverted compiler: plain `-fsyntax-only` runs 0/10 crashes (heap-layout luck —
  matches the field non-determinism); `MallocScribble=1` runs **5/5 SIGSEGV** with the
  faulting frame `clang::Sema::PopExpressionEvaluationContext` — the exact field
  signature from the nlohmann/json binding crash.
- Fixed compiler: repro + the new libcxx test pass, including under `MallocScribble=1`;
  binder suite 50/50; json corpus run at outcome E; clang/test/Reflection 16/16.
