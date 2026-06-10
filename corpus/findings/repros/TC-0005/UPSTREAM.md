# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/292
- **PR:** https://github.com/bloomberg/clang-p2996/pull/293 — branch
  `reflect-attributed-fn-type-queries` on `Cfretz244/llvm-project` (commit `025144a`:
  `06b9344` cherry-picked onto their `p2996` tip `837da39` == our local base, so the PR
  code is byte-for-byte what the corpus validated; no internal finding references in
  code comments).

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here: the six qualifier `static_assert`s fail at base `837da39eb88c`
  (predicates silently answer false); `-DTYPE_QUERIES` adds the three
  "not an integral constant expression" / "cannot introspect the parameters of a
  non-function type" failures from `return_type_of`/`parameters_of`/
  `has_ellipsis_parameter` on the attributed function TYPE. All pass with the fix.
  Removing `[[clang::lifetimebound]]` is the control: base passes everything.
- The field shape (qualifier probe over `members_of(^^absl::StatusOr<int>)` `value()`
  proxies' underlyings against real Abseil): all-false at the pre-fix toolchain,
  correct `const&`/`&`/`const&&`/`&&` with the fix.
- Minimization (recorded in the finding): proxies / instantiated class templates /
  injected-class-name `using` qualifiers are all incidental — a plain non-template
  class with lifetimebound methods reproduces on direct reflections. This resolves
  the "qualifier predicates through StatusOr proxies" observation deferred from
  #286 / #290 (originally TC-0003 addendum #2).
- New regression test `attributed-function-type-queries.pass.cpp`: 15 failures at
  base, passes with the fix.
- PR-branch compiler (fix on plain `837da39`): repro + test pass;
  `clang/test/Reflection` 15/16 — identical to pristine base (`splice-exprs.cpp`
  pre-existing).
- Full local toolchain (reflection-p2996 @ `06b9344`): lit 16/16, binder suite 50/50
  with modules recompiled, `abseil_statusor` + `json` corpus gates at E.
