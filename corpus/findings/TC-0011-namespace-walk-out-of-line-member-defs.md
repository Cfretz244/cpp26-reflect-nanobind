# TC-0011 — namespace members_of derails on out-of-line class-member definitions in re-opened blocks

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project (`clang/lib/AST/ExprConstantMeta.cpp`, `isReflectableDecl` +
  `findIterableMember`); regression test
  `llvm-project/libcxx/test/std/experimental/reflection/namespace-members-out-of-line-defs.pass.cpp`;
  prepared for upstream (draft in `repros/TC-0011/UPSTREAM.md`)
- **Track:** toolchain (clang-p2996, pinned by this repo), namespace member
  enumeration (`members_of`)
- **Found via:** `corpus/runs/eigen` (Eigen 5.0.1), Gate 4 — the binder's
  `bind_free_operators` lifts `members_of(^^Eigen)` into a
  `define_static_array`; the compiler died with `Assertion failed: (Val &&
  "isa<> used on a null pointer")` in `MetaActionsImpl::Substitute`
  (substituting the lifted array's variable template with the member
  reflections as arguments). Bisection of the lifted members pinned member
  #68: `Eigen::canonicalEulerAngles` — not a namespace member at all, but the
  out-of-line definition pattern of `MatrixBase<Derived>::canonicalEulerAngles`
  (Eigen/src/Geometry/EulerAngles.h re-opens `namespace Eigen` with that
  definition as its FIRST declaration).
- **Repro:** `corpus/findings/repros/TC-0011/repro.cpp` (standalone; the
  static_asserts show both the wrong member and the dropped members).

## Symptom

Two defects compound when a re-opened namespace block BEGINS with an
out-of-line class-member definition (lexically in the namespace, semantically
in the class):

1. **The definition is yielded as a namespace member.**
   `isReflectableDecl`'s redeclaration filter walks the redecl chain and asks
   "is `D` the first declaration in its own LEXICAL context?" — and the
   out-of-line definition IS the first (only) redeclaration at namespace
   scope, so it passes. For a class template, what gets reflected is the
   dependent definition PATTERN; using that reflection (display_string_of,
   or as a template argument of the `define_static_array` backing variable
   template) crashes with `isa<> used on a null pointer`.
2. **Every remaining namespace member is silently dropped.** Stepping FROM the
   definition consults its SEMANTIC DeclContext (the class) and follows the
   class's multi-decl-context chain (`getPrevMultDCDeclInSemaContext`) out of
   the namespace walk entirely. In the minimized repro, `members_of(^^n)`
   returns exactly ONE entry — the bogus `g` — for a namespace with three real
   members. Against real Eigen, the walk saw 69 of 125 members.

The walk's in-block stepping is unaffected (it already skips decls whose
semantic context differs); only LANDING on such a decl — the cross-block hop
`D = *NSDecl->decls_begin()` and the initial `decls_begin()` — exposes both
defects.

## Fix

Two complementary changes in `ExprConstantMeta.cpp`:

- `isReflectableDecl`: reject any decl whose semantic `DeclContext` is a
  `CXXRecordDecl` while its lexical context differs — an out-of-line member
  definition is a pure redeclaration of a CLASS member, enumerable only
  through its class. (Class walks never produce lexical-elsewhere decls, and
  the namespace-side `getLastMultDCSemaDecl` chain only carries decls whose
  semantic context is a NAMESPACE, so nothing legitimate matches.)
- `findIterableMember`: when stepping FROM such a decl, use its LEXICAL
  context as the iteration context, so the walk continues through the
  namespace block instead of departing into the class's chain. Decls whose
  semantic context is a namespace (the legitimate
  `getLastMultDCSemaDecl`/`getPrevMultDCDeclInSemaContext` multi-context
  chain, e.g. `void ns::f() {}` at TU scope) are deliberately NOT rewritten.

## Validation

- `repro.cpp`: at the pre-fix toolchain the walk yields only `g`; with the
  fix it yields `Tmpl`/`h`/`k` and no `g`.
- Regression test `namespace-members-out-of-line-defs.pass.cpp` (single-block,
  doubly re-opened, template + non-template definition shapes, plus
  no-duplication through the class walk): passes with the fix; the re-opened
  cases fail without.
- Real-Eigen scale: `define_static_array(members_of(^^Eigen))` crashed before;
  with the fix it lifts cleanly — and yields 125 members where the broken walk
  had silently produced 69.
- Binder suite 54/54 on the rebuilt toolchain.
