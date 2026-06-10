# READY TO FILE — bloomberg/clang-p2996 issue + PR (prepared 2026-06-10)

Everything verified; filing was blocked by the session's permission layer, not by
verification. Full issue body: `upstream-issue.md`; full PR body: `upstream-pr.md`
(replace `#ISSUE_TC8` with the filed issue number).

- **Issue title:** `Itanium mangler ICE ("Can't mangle a deduction guide name!") mangling a reflection of a deduction guide as a template argument / define_static_array element`
- **PR title:** `[clang][reflection] Mangle deduction-guide reflections instead of hitting unreachable`
- **Branch:** `reflect-deduction-guide-mangling` — a local branch of the `llvm-project/`
  submodule, **stacked on PR #287's branch** (`reflect-fn-template-nttp-mangling` —
  this change extends the same `mangleReflection` hash block and reuses its ODR-hash
  convention/include): the reflection-p2996 fix commit (`3cc8232b2e07`) cherry-picked on
  top, message reworded, internal finding references scrubbed. **Verified standalone**:
  built Release+assertions; the new regression test, `repro.cpp` (full matrix), and
  #287's own test pass with the branch compiler.
- The binder side also gained a belt-and-suspenders guide filter
  (`namespace_members_for_binding` in nb_reflect.h) so it works on unpatched
  toolchains; the compiler fix is required regardless (no silent ICEs).

## Filing commands (from the umbrella root)

```bash
git -C llvm-project push origin reflect-deduction-guide-mangling
gh issue create -R bloomberg/clang-p2996 \
  --title 'Itanium mangler ICE ("Can'"'"'t mangle a deduction guide name!") mangling a reflection of a deduction guide as a template argument / define_static_array element' \
  --body-file corpus/findings/repros/TC-0008/upstream-issue.md
# note the issue number N, then:
sed -i '' 's/#ISSUE_TC8/#N/g' corpus/findings/repros/TC-0008/upstream-pr.md
gh pr create -R bloomberg/clang-p2996 --base p2996 \
  --head Cfretz244:reflect-deduction-guide-mangling \
  --title '[clang][reflection] Mangle deduction-guide reflections instead of hitting unreachable' \
  --body-file corpus/findings/repros/TC-0008/upstream-pr.md
# after filing, also fix the #PR_TC8 placeholder in TC-0009's PR body:
#   sed -i '' 's/#PR_TC8/#<this PR number>/g' corpus/findings/repros/TC-0009/upstream-pr.md
```

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here, full matrix: `-DGUIDE -c` ICE'd at base
  (`Can't mangle a deduction guide name!`, ItaniumMangle.cpp:1774; exit 134) and is
  clean with the fix; no-guide `-c` control clean both ways; `-DGUIDE -fsyntax-only`
  clean both ways (front-end fine — it is the mangler).
- Enumeration reality check that shaped the fix: `members_of` over the repro namespace
  yields FOUR guides (2 explicit + Sema's implicit per-constructor and copy guides),
  with the implicit per-constructor guide structurally identical to explicit guide #1 —
  a plain structural hash folds them; `isImplicit()` + deduction-candidate kind in the
  hash keeps all four pairwise distinct (asserted at runtime in the new test).
- New regression test `deduction-guide-reflection-mangling.pass.cpp`: passes with the
  fix; #287's `substitute-nested-dependent.pass.cpp` still passes on the stack.
- `clang/test/Reflection` via `LIT_FILTER=Reflection check-clang`: 16/16 with the fix.
- Full local toolchain: binder suite 53/53 (including a namespace-scope guide planted in
  the test namespace); `corpus/runs/expected` — whose RECORDED Gate-4 failure this was
  (binding ANY tl class died at codegen) — at outcome E, 16/16 differential tests.
