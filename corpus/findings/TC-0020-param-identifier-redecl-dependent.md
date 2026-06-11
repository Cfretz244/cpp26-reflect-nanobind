# TC-0020 (candidate): parameter identifier_of is redeclaration-dependent

- **Status**: open (documented; run-local surface_diff_ignore in eigen).
- **Symptom**: `identifier_of(parameters_of(fn)[i])` returns DIFFERENT names
  in different translation units for a function whose declaration and
  out-of-line definition name the parameter differently. Field shape:
  Eigen declares `DenseBase::setConstant(const Scalar& value)`
  (DenseBase.h:341) and defines it with `val` (CwiseNullaryOp.h:347). The
  constexpr binding TU reads `val`; the emit generator TU reads `value` --
  the only surface mismatch across the whole 36-run corpus
  (`setConstant.doc: val vs value`, eigen Gate 6b).
- **Spec angle**: P3096 intends parameter-name queries to be meaningful only
  when consistent across redeclarations (a consistency predicate /
  unnamed-fallback); the pinned toolchain exposes whichever redeclaration
  its AST traversal last merged, making the result evaluation-context-
  dependent. No `has_consistent_identifier` exists to gate on.
- **Repro sketch**: declare `void f(int a);` + define `void f(int b) {}`;
  identifier_of(parameters_of(^^f)[0]) differs by whether the definition was
  instantiated/parsed before the query (exact trigger conditions TBD during
  minimization).
- **Possible fixes**: (1) toolchain: resolve parameter names through the
  canonical declaration (deterministic), or implement P3096's consistency
  rule (inconsistent -> unnamed); (2) binder: once a consistency predicate
  exists, skip nb::arg for inconsistently-named parameters in BOTH backends.
- **Interim**: eigen's meta.toml carries a surface_diff_ignore for exactly
  this attribute, with this finding as the justification. Behavior is
  unaffected except which keyword name Python callers may use for that one
  method.
