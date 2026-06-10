# DRAFT — not yet filed (prepared 2026-06-10, eigen run)

Planned: bloomberg/clang-p2996 issue + PR, same flow as TC-0005..0010.

## Proposed issue title

`members_of(^^ns) yields an out-of-line member-definition pattern and drops
every other member when a re-opened namespace block starts with one`

## Proposed issue body (summary)

```cpp
namespace n {
template <class T> struct Tmpl { int g() const; };
inline int h() { return 3; }
}
namespace n {                                 // re-opened; FIRST decl is the
template <class T> int Tmpl<T>::g() const {   // out-of-line definition PATTERN
  return 2;
}
inline int k() { return 4; }
}
// members_of(^^n) returns exactly ONE entry: g (the definition pattern).
// Tmpl, h, k are silently dropped. Using the g reflection (display_string_of,
// or as a template argument) hits "isa<> used on a null pointer" for
// dependent signatures.
```

Cause (clang/lib/AST/ExprConstantMeta.cpp): the namespace walk's cross-block
hop lands on `*NSDecl->decls_begin()` unchecked; (1) `isReflectableDecl`'s
redeclaration filter passes the definition because it is the first
redeclaration in its own LEXICAL context, and (2) stepping from it consults
its SEMANTIC DeclContext (the class) and follows
`getPrevMultDCDeclInSemaContext` out of the namespace entirely.

Field shape: Eigen defines most facade members out-of-line in re-opened
`namespace Eigen` blocks (EulerAngles.h opens with the
`MatrixBase<Derived>::canonicalEulerAngles` definition), so any namespace
sweep over `^^Eigen` either crashes or silently sees a gutted namespace
(69 of 125 members).

## Proposed fix

- `isReflectableDecl`: reject decls whose semantic DeclContext is a
  `CXXRecordDecl` while the lexical context differs (an out-of-line member
  definition is a redeclaration of a CLASS member, enumerable only through
  the class).
- `findIterableMember`: step FROM such a decl through its LEXICAL context.
  Decls whose semantic context is a NAMESPACE (the getLastMultDCSemaDecl
  multi-context chain, e.g. `void ns::f() {}` at TU scope) are not rewritten.

Regression test:
`libcxx/test/std/experimental/reflection/namespace-members-out-of-line-defs.pass.cpp`.

## Validation done locally

- Minimized repro flips from 1 bogus member to the 3 real ones.
- New regression test passes with fix, fails (re-opened cases) without.
- Real Eigen: `define_static_array(members_of(^^Eigen))` crashed; now lifts
  125 members cleanly.
- libcxx reflection suite + downstream binder suite green.
