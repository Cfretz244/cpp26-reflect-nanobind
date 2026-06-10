# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/296
- **PR:** https://github.com/bloomberg/clang-p2996/pull/297 — branch
  `reflect-implicit-instantiation-completion` on `Cfretz244/llvm-project`.
  Filed bodies: `upstream-issue.md` / `upstream-pr.md` (here, verbatim).

- **Issue title:** `` `members_of` instantiates member function definitions when it triggers the class template specialization's instantiation — valid types with lazily-ill-formed member bodies are wrong-rejected (order-dependent) ``
- **PR title:** `[clang][reflection] Complete reflected specializations with implicit-instantiation semantics`
- **Branch:** `reflect-implicit-instantiation-completion` — a local branch of the
  `llvm-project/` submodule: the reflection-p2996 fix commit (`b5836f1550dc`)
  cherry-picked onto bloomberg `p2996` tip `837da39eb88c`, message reworded to upstream
  style. **Verified standalone**: built Release+assertions on that base; the new
  regression test and `repro.cpp` (both orderings) pass with the branch compiler.

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here: `error: no member named 'm_val' in 'Exp<void>'` (from inside the
  `members_of` iteration) at base; clean with `-DPREINSTANTIATE` at base (the
  order-dependence smoking gun). With the fix: clean in BOTH orderings.
- New regression test `members-of-lazily-ill-formed-bodies.pass.cpp`: five distinct
  templates so each completing metafunction (`members_of`,
  `nonstatic_data_members_of`, `bases_of`, `is_complete_type`, `size_of`) is the FIRST
  instantiation trigger; passes with the fix.
- Body-needing consumers spot-checked green with the fix: `reflect-invoke.pass.cpp`,
  `to-and-from-values.pass.cpp`, `consteval-reentrant-instantiation.pass.cpp`,
  `members-and-subobjects.pass.cpp`.
- PR-branch compiler (fix on plain `837da39eb88c`): repro + test pass standalone.
- `clang/test/Reflection` via `LIT_FILTER=Reflection check-clang`: 16/16 with the fix;
  the three machine-local pre-existing .pass.cpp failures fail identically at pristine
  base (audited via a stash-and-rebuild baseline) — no new failures.
- Full local toolchain: binder suite 53/53; `corpus/runs/expected` at outcome E with
  `tl::expected<void, std::string>` in the bind set (this bug produced nine hard errors
  on bare enumeration of that spec and was why it had been dropped).
