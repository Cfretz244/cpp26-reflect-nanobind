# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/309
- **PR:** https://github.com/bloomberg/clang-p2996/pull/310 — branch
  `reflect-dependent-splice-classification` on `Cfretz244/llvm-project` (commit
  `5dd9a31424bf`: `fc2ca0dc74ef` cherry-picked onto their `p2996` tip `837da39eb88c`
  == our local base, so the PR code is byte-for-byte what the corpus validated;
  commit message cleaned of internal finding references; code comments already clean).
- **Tracking:** listed on the campaign tracking issue
  https://github.com/bloomberg/clang-p2996/issues/308 under category
  **E (constant-evaluator robustness)** — tick its checkbox when the PR merges.

## Validation evidence behind the filing (all on this laptop)

- **PR-state compiler** (pristine base `837da39` + ONLY this fix, built
  Release+assertions in `/tmp/p2996-validate-build` from the
  `reflect-dependent-splice-classification` worktree):
  - `repro.cpp` compiles clean; regression test
    `auto-nttp-dependent-splice-requires.pass.cpp` passes (run manually — the
    runtimes lit harness has no test-suite-install).
  - **Negative control:** reverting just `clang/lib/AST/ExprClassification.cpp` to
    base in the same build reproduces the parse-time assertion
    (`Casting.h:662 dyn_cast on a non-existent value`); restoring the fix cures it.
  - `clang/test/Reflection`: 15/16 — identical to pristine base at `837da39`
    (`splice-exprs.cpp` pre-existing, same baseline as the TC-0010..0012 filings).
- **Local toolchain** (reflection-p2996 + all prove-out fixes): repro clean,
  regression test passes, `clang/test/Reflection` 16/16, binder suite 58/58,
  full corpus 28/28 outcome E.

## Provenance

Found by the DRIVER (not a corpus run agent) during wave 1 of the Phase 3 parallel
fan-out, implementing BINDER-0020's value-readability probe
(`requires { typename probe<([:mem:])>; }` with an `auto` NTTP). The binder ships a
fixed-type (`long double`) NTTP probe as the workaround, so it works on unpatched
toolchains.
