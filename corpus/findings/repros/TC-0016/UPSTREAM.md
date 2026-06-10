# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10, wave-2 batch)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/313
- **PR:** https://github.com/bloomberg/clang-p2996/pull/317 — branch `reflect-linkage-spec-member-walk` on
  `Cfretz244/llvm-project` (tip `284fe817faf3`), based on STACKED on PR #306's branch (the TC-0011 namespace-walk fix this extends). Commit message cleaned of
  internal finding tags; code comments already clean.
- **Tracking:** listed on https://github.com/bloomberg/clang-p2996/issues/308 under its
  root-cause category — tick the checkbox when the PR merges. #308 also now carries the
  validation-breadth table (36-run corpus, 24 libraries, 300 differential tests) this
  batch's fixes were validated against.

## Validation evidence (this laptop, Release+assertions AArch64)

- **PR-state compiler** (built in `/tmp/p2996-batch-build` from the branch): repro and members-of-linkage-spec-typedef-tag.pass.cpp (TU + namespace shapes) pass on the PR-state compiler; reverting just ExprConstantMeta.cpp to the stack base reproduces the truncation. NOTE: the regression test filters by identifier BEFORE probing is_class_type so it stays independent of the separately-filed NEON gap (#314).
- `clang/test/Reflection`: 15/16 on the PR-state compiler — identical to pristine base
  (`splice-exprs.cpp` pre-existing).
- **Local toolchain** (reflection-p2996 + all prove-out fixes): repro clean, regression
  tests pass manually, `clang/test/Reflection` 16/16, binder suite 64/64, full corpus
  **36/36 outcome E** on the final compiler.
