# DRAFT — not yet filed (prepared 2026-06-10, eigen run)

Planned: bloomberg/clang-p2996 issue + PR, same flow as TC-0005..0009 (cherry-pick
the fix commit onto their `p2996` tip on a branch of `Cfretz244/llvm-project`,
no internal finding references in code comments).

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
