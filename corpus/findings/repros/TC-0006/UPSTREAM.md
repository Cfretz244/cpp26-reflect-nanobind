# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/294
- **PR:** https://github.com/bloomberg/clang-p2996/pull/295 — branch
  `reflect-substitute-failure-report` on `Cfretz244/llvm-project`.
  Filed bodies: `upstream-issue.md` / `upstream-pr.md` (here, verbatim).

- **Issue title:** `` `can_substitute`/`substitute` crash (SIGSEGV) instead of reporting failure when substitution forms an invalid type inside a template-id (e.g. reference to void) ``
- **PR title:** `[clang][reflection] Report substitution failure instead of crashing when substitution forms an invalid type in the declaration`
- **Branch:** `reflect-substitute-failure-report` — a local branch of the `llvm-project/`
  submodule (commit `6a8b94536bc4`): the reflection-p2996 fix commit (`44a946784fee`)
  cherry-picked onto bloomberg `p2996` tip `837da39eb88c`, message reworded to upstream
  style, internal finding references scrubbed. **Verified standalone**: built
  Release+assertions on that base; the new regression test and `repro.cpp` pass with the
  branch compiler.

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here: SIGSEGV at base `837da39eb88c` (stack ends in
  `MetaActionsImpl::Substitute(FunctionTemplateDecl*, ...)`); compiles clean with the
  fix (`-fsyntax-only` suffices). The alias-template sibling leaked
  `cannot form a reference to 'void'` and went non-constant; the variable-template
  sibling tripped the post-`Substitute` assert; both now report `false`.
- New regression test `can-substitute-invalid-type-formation.pass.cpp` + the diagnosing
  case in `substitute.verify.cpp`: pass with the fix (verify file run with
  `-Xclang -verify -Xclang -verify-ignore-unexpected=note`, the libc++ harness contract).
- PR-branch compiler (fix on plain `837da39eb88c`): repro + tests pass standalone.
- `clang/test/Reflection` via `LIT_FILTER=Reflection check-clang`: 16/16 with the fix.
- Pre-existing-failure audit: the three reflection .pass.cpp files that fail on this
  machine (`anon-union`, `p3096-fn-parameters`, `p3385-function-attributes` — the last
  uses a flag this clang does not accept) fail identically on a pristine-base build;
  no new failures from this change.
- Full local toolchain (reflection-p2996): binder suite 53/53; `corpus/runs/expected`
  at outcome E **with `tl::expected<void, std::string>` in the bind set** — the binder's
  `can_substitute({})` probes on `swap<OT=void>`/`value<U=void>` now gracefully report
  not-substitutable (this bug was why the void spec had been dropped).
