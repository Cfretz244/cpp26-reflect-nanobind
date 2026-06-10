# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/304
- **PR:** https://github.com/bloomberg/clang-p2996/pull/307 — branch
  `reflect-is-complete-type-desugar` on `Cfretz244/llvm-project` (commit `785fbb9`:
  `c5db481` cherry-picked onto their `p2996` tip `837da39` == our local base, so
  the PR code is byte-for-byte what the corpus validated; commit message cleaned of
  internal finding references; code comments were already clean).
- **Tracking:** listed on the campaign tracking issue
  https://github.com/bloomberg/clang-p2996/issues/308 (keep its checkbox/state
  current as the PR merges).

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` + regression test `is-complete-type-alias-sugar.pass.cpp` compiled and run against a
  PR-state compiler (base `837da39` + the three eigen-run fixes, built in
  `toolchain-build` from the `reflect-validation-tc10-12` branch): all pass;
  each fails at pristine base per the repro headers.
- `clang/test/Reflection`: 15/16 on the PR-state compiler — identical to
  pristine base (`splice-exprs.cpp` pre-existing).
- Full local toolchain (reflection-p2996): binder suite 54/54, full corpus
  20/20 outcome E.

## Proposed issue title

`is_complete_type answers false for an instantiable specialization named
through a type alias (and the answer is order-dependent)`

## Proposed issue body (summary)

```cpp
template <class T> struct Box { T v; };
using BoxInt = Box<int>;
static_assert(std::meta::is_complete_type(^^BoxInt));     // fails
static_assert(std::meta::is_complete_type(^^Box<long>));  // passes
using BoxLong = Box<long>;
static_assert(std::meta::is_complete_type(^^BoxLong));    // passes (only
                                                          // because the line
                                                          // above instantiated)
```

`is_complete_type` (clang/lib/AST/ExprConstantMeta.cpp) passes the sugared
type to `findTypeDecl`, which for a `TypedefType` returns the alias
declaration; `EnsureInstantiated(TypedefDecl)` is a no-op, so a
never-yet-referenced specialization reads as incomplete. The `members_of`
family desugars aliases (`desugarType(..., UnwrapAliases=true, ...)`) before
`findTypeDecl`; `is_complete_type` should too. Same theme as #292 (sugar-blind
metafunctions), different site.

## Proposed fix

Desugar before `findTypeDecl` and test completeness on the desugared type
(three-line change mirroring `get_begin_member_decl_of`). Regression test:
`libcxx/test/std/experimental/reflection/is-complete-type-alias-sugar.pass.cpp`.

## Validation done locally

- Repro flips from failing q1 to all-pass.
- Regression test covers alias / member-typedef / forward-declared-only /
  order-dependence / non-template control shapes.
- Downstream: a completeness-gated reflection walk over spdlog (whose seeds
  and signatures are alias-heavy) went from silently dropping types to
  correct.
