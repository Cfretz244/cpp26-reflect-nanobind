# DRAFT — not yet filed (prepared 2026-06-10, eigen run)

Planned: bloomberg/clang-p2996 issue + PR, same flow as TC-0005..0011.

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
