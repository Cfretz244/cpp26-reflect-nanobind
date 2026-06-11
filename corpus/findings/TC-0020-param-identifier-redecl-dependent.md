# TC-0020: parameter identifier_of was instantiation-state-dependent

- **Status**: fixed locally (llvm-project submodule,
  `clang/lib/AST/ExprConstantMeta.cpp` `getParameterName`); upstream filing
  pending.
- **Root cause** (refined during minimization): the P3096 consistency walk
  (`getParameterName` walks the function's redeclaration chain and reports
  no identifier when names differ) ran over the INSTANTIATION's chain. For
  an instantiated member, instantiating the out-of-line definition REPLACES
  the parameters on the same FunctionDecl -- no redeclaration is added --
  so the walk saw exactly one name: whichever redeclaration happened to be
  instantiated last. Same entity, same query, different answer depending on
  instantiation state.
- **Fix**: walk the template instantiation PATTERN's declaration chain
  (which carries both the in-class declaration and the out-of-line
  definition regardless of instantiation state); skipped when the pattern
  contains a parameter pack (its parameter list does not line up
  index-for-index; pack-substituted parameters are already filtered).
  Result: an inconsistently-named parameter deterministically has NO
  identifier; consistently-named parameters keep their name.
- **Minimized trigger**: `template void S<int>::f(int);` (explicit
  instantiation of the definition) before the query flips the pre-fix
  answer from the declaration's name to the definition's.
- **Found by**: eigen's Gate 6b surface diff -- the only mismatch across
  the 36-run corpus. Eigen declares `DenseBase::setConstant(const Scalar&
  value)` (DenseBase.h) and defines it with `val` (CwiseNullaryOp.h); the
  constexpr binding TU (whose lambdas odr-use the member, instantiating the
  definition by query time) said `val`, the emit generator TU (reflection
  only, definition never instantiated) said `value`.
- **Binder impact**: with the fix, setConstant's parameter is unnamed in
  BOTH lanes (the binder's has_identifier guard already handles unnamed
  params); eigen's interim `surface_diff_ignore` is removed.
- **Repro**: `repros/TC-0020/repro.cpp` (both variants assert the
  deterministic post-fix behavior + a consistent-name control). Regression
  test:
  `llvm-project/libcxx/test/std/experimental/reflection/param-name-consistency-instantiation.pass.cpp`.
- **Category for #308**: B-adjacent (a metafunction answering from the
  wrong redeclaration chain) -- closest existing family is B; file there
  with a note, or extend the taxonomy if reviewers prefer.
