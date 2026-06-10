# FILED — bloomberg/clang-p2996 issue + PR (2026-06-10)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/302
- **PR:** https://github.com/bloomberg/clang-p2996/pull/305 — branch
  `reflect-namespace-equality-canonical-decl` on `Cfretz244/llvm-project` (commit `3729fb1`:
  `3b1fda0` cherry-picked onto their `p2996` tip `837da39` == our local base, so
  the PR code is byte-for-byte what the corpus validated; commit message cleaned of
  internal finding references; code comments were already clean).
- **Tracking:** listed on the campaign tracking issue
  https://github.com/bloomberg/clang-p2996/issues/308 (keep its checkbox/state
  current as the PR merges).

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` + regression test `namespace-reflection-equality-reopened.pass.cpp` compiled and run against a
  PR-state compiler (base `837da39` + the three eigen-run fixes, built in
  `toolchain-build` from the `reflect-validation-tc10-12` branch): all pass;
  each fails at pristine base per the repro headers.
- `clang/test/Reflection`: 15/16 on the PR-state compiler — identical to
  pristine base (`splice-exprs.cpp` pre-existing).
- Full local toolchain (reflection-p2996): binder suite 54/54, full corpus
  20/20 outcome E.

## Proposed issue title

`parent_of(^^member) != ^^ns when the member is declared in a re-opened
namespace block (reflection equality not canonicalized for namespaces)`

## Proposed issue body (summary)

P2996 requires two reflections to compare equal when they designate the same
entity. For namespaces this fails across redeclarations:

```cpp
namespace c { namespace d {} }
namespace c { namespace d { template <class T> struct R {}; } }

static_assert(std::meta::parent_of(^^c::d::R) == ^^c::d);   // fails
```

`^^c::d` wraps the first `NamespaceDecl`, `parent_of(^^c::d::R)` wraps the
re-opened block's `NamespaceDecl`, and `profileReflection`
(clang/lib/AST/APValue.cpp) profiles the raw decl pointer for
`ReflectionKind::Namespace` — unlike `ReflectionKind::Template`, which
canonicalizes via `getCanonicalDecl()`. Any namespace-membership test of the
form `parent_of(x) == ^^ns` silently answers false for entities declared after
the namespace's first block. Field shape: Eigen re-opens `Eigen::internal` in
nearly every header, so enclosing-namespace classification of Eigen entities
is wrong for essentially all of them.

## Proposed fix

In `profileReflection`, profile the canonical declaration for namespaces:

```cpp
case ReflectionKind::Namespace:
  ID.AddPointer(V.getReflectedNamespace()->getCanonicalDecl());
  return;
```

`NamespaceAliasDecl` canonicalizes to its own first declaration (an alias
stays distinct from its target — `^^alias != ^^ns` while
`underlying_entity_of(^^alias) == ^^ns`); `TranslationUnitDecl` is self-
canonical. Regression test:
`libcxx/test/std/experimental/reflection/namespace-reflection-equality-reopened.pass.cpp`.

## Validation done locally

- `repro.cpp`: fails (q2) at base, passes with fix.
- New regression test: re-opened cases fail at base, all pass with fix;
  alias distinction pinned.
- Full binder suite (54 tests) green on the rebuilt toolchain; eigen corpus
  run's `^^Eigen::internal` exclusion behaves correctly with the fix.
