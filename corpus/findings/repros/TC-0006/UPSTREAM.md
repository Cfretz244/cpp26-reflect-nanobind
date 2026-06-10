# READY TO FILE — bloomberg/clang-p2996 issue + PR (prepared 2026-06-10)

Everything verified; filing was blocked by the session's permission layer, not by
verification. Full issue body: `upstream-issue.md`; full PR body: `upstream-pr.md`
(replace `#ISSUE_TC6` with the filed issue number).

- **Issue title:** `` `can_substitute`/`substitute` crash (SIGSEGV) instead of reporting failure when substitution forms an invalid type inside a template-id (e.g. reference to void) ``
- **PR title:** `[clang][reflection] Report substitution failure instead of crashing when substitution forms an invalid type in the declaration`
- **Branch:** `reflect-substitute-failure-report` — a local branch of the `llvm-project/`
  submodule (commit `6a8b94536bc4`): the reflection-p2996 fix commit (`44a946784fee`)
  cherry-picked onto bloomberg `p2996` tip `837da39eb88c`, message reworded to upstream
  style, internal finding references scrubbed. **Verified standalone**: built
  Release+assertions on that base; the new regression test and `repro.cpp` pass with the
  branch compiler.

## Filing commands (from the umbrella root)

```bash
git -C llvm-project push origin reflect-substitute-failure-report
gh issue create -R bloomberg/clang-p2996 \
  --title '`can_substitute`/`substitute` crash (SIGSEGV) instead of reporting failure when substitution forms an invalid type inside a template-id (e.g. reference to void)' \
  --body-file corpus/findings/repros/TC-0006/upstream-issue.md
# note the issue number N, then:
sed -i '' 's/#ISSUE_TC6/#N/g' corpus/findings/repros/TC-0006/upstream-pr.md
gh pr create -R bloomberg/clang-p2996 --base p2996 \
  --head Cfretz244:reflect-substitute-failure-report \
  --title '[clang][reflection] Report substitution failure instead of crashing when substitution forms an invalid type in the declaration' \
  --body-file corpus/findings/repros/TC-0006/upstream-pr.md
```

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
