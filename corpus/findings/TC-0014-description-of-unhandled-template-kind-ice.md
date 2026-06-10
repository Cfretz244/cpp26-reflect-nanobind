# TC-0014 — DescriptionOf crashed on builtin-template reflections ("unhandled template kind")

- **Status:** FIXED locally (`clang/lib/AST/ExprConstantMeta.cpp`, `DescriptionOf`); upstream filing this wave.
- **Found via:** corpus/runs/box2d (wave 2, THE B-blocker: every box2d public type is a
  global-namespace class) + independently corpus/runs/sqlitecpp (over-minimal repro);
  dedup keys `return-type-of-description-of-unhandled-template-kind-ice`,
  `toolchain-descriptionof-template-kind-unreachable`.
- **Symptom:** reflecting ANY class in the GLOBAL namespace ICE'd at constant evaluation:
  `UNREACHABLE "unhandled template kind"` (ExprConstantMeta.cpp:1772). The binder's
  free-operator scan walks the enclosing namespace; the global namespace enumerates
  clang's BUILTIN templates (`__make_integer_seq` et al.), a metafunction's RECOVERED
  error path described one, and DescriptionOf's `ReflectionKind::Template` switch had no
  arm for `BuiltinTemplateDecl`.
- **Fix:** added BuiltinTemplateDecl/TemplateTemplateParmDecl arms + a generic "a template"
  fallback — a description helper feeding diagnostics must degrade, never crash. With the
  fix the original binder TU compiles CLEAN (the error had always been handled upstream;
  only building its message crashed).
- **Repro:** `repros/TC-0014/` (direct 15-line + the 4-line binder shape); regression test
  `description-of-template-kinds.verify.cpp` (asserts the clean diagnostic, incl. the new
  "a builtin template" wording). Category **E** (constant-evaluator robustness).
