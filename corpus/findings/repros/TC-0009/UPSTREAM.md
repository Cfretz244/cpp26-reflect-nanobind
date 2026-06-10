# READY TO FILE — bloomberg/clang-p2996 issue + PR (prepared 2026-06-10)

Everything verified; filing was blocked by the session's permission layer, not by
verification. Full issue body: `upstream-issue.md`; full PR body: `upstream-pr.md`
(replace `#ISSUE_TC9` with the filed issue number and `#PR_TC8` with the TC-0008 PR
number).

- **Issue title:** `Same-named member function templates of a class template specialization with identical template heads mangle identically as reflection NTTPs — silent linkonce_odr fold (gap in #286's fix)`
- **PR title:** `[clang][reflection] Discriminate same-headed member-template reflections of a specialization in NTTP mangling`
- **Branch:** `reflect-spec-context-nttp-mangling` — a local branch of the
  `llvm-project/` submodule, **stacked on `reflect-deduction-guide-mangling`** (which
  stacks on PR #287; all three amend the same `mangleReflection` hash block): the
  reflection-p2996 fix commit (`54393db25fea`) cherry-picked on top, message reworded,
  internal finding references scrubbed. **Verified standalone**: built
  Release+assertions; the new regression test, `repro.cpp` (matrix incl. the
  namespace-scope control), and both upstack tests (#287's + the guide test) pass with
  the branch compiler.

## Filing commands (from the umbrella root; file AFTER TC-0008's PR so #PR_TC8 is known)

```bash
git -C llvm-project push origin reflect-spec-context-nttp-mangling
gh issue create -R bloomberg/clang-p2996 \
  --title 'Same-named member function templates of a class template specialization with identical template heads mangle identically as reflection NTTPs — silent linkonce_odr fold (gap in #286'"'"'s fix)' \
  --body-file corpus/findings/repros/TC-0009/upstream-issue.md
# note the issue number N (and TC-0008's PR number M), then:
sed -i '' 's/#ISSUE_TC9/#N/g; s/#PR_TC8/#M/g' corpus/findings/repros/TC-0009/upstream-pr.md
gh pr create -R bloomberg/clang-p2996 --base p2996 \
  --head Cfretz244:reflect-spec-context-nttp-mangling \
  --title '[clang][reflection] Discriminate same-headed member-template reflections of a specialization in NTTP mangling' \
  --body-file corpus/findings/repros/TC-0009/upstream-pr.md
```

## Validation evidence behind the filing (all on this laptop)

- Found IN THE FIELD by `corpus/runs/expected`'s differential suite on its first
  execution after TC-0008 + BINDER-0012 landed: outcome D, `AttributeError: ... has no
  attribute 'value'` — `expected<T,E>::value()` had silently never bound. A direct
  probe showed all four `value()` sibling reflections sharing ONE mangled name
  (`...5valueIEE$1393736719$...`), i.e. `error: definition with same mangled name`
  when pinned explicitly.
- Root cause confirmed in source: `ODRHash::AddFunctionDecl` returns early for decls in
  specialization context ("Skip functions that are specializations or in specialization
  context", ODRHash.cpp), so only the (identical) template heads were hashed. absl's
  TC-0004 siblings escaped because their heads differ.
- `repro.cpp` here: `-c` fails with the duplicate-mangled-name error pre-fix, clean +
  runs to exit 0 with the fix; `-fsyntax-only` clean both ways; `-DFREE_CONTROL`
  (same signatures at namespace scope — no specialization context) clean both ways.
- New regression test `fn-template-nttp-mangling-spec-context.pass.cpp` (four-sibling
  value() shape + a ref-qualifier-only pair, runtime distinctness): passes with the fix.
- PR-branch compiler (stack tip): repro + new test + both upstack regression tests pass.
- `clang/test/Reflection` via `LIT_FILTER=Reflection check-clang`: 16/16 with the fix.
- Full local toolchain: binder suite 53/53; `corpus/runs/expected` at outcome E with
  `value()` bound and its throw-path differential (`bad_expected_access` →
  RuntimeError) passing.
