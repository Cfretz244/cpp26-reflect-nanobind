# TC-0015 — deduction-guide SPECIALIZATION reflections ICE'd the Itanium mangler (Declaration-kind path)

- **Status:** FIXED locally (`clang/lib/AST/ItaniumMangle.cpp`, `mangleReflection`
  Declaration case); upstream filing this wave. TC-0008's sibling: that fix covered
  Template-kind guide reflections; a guide SPECIALIZATION (e.g. via `substitute` on the
  guide template) is Declaration-kind and routed through `mangle()` →
  `mangleFunctionEncoding` → `mangleUnqualifiedName` → `llvm_unreachable("Can't mangle a
  deduction guide name!")` (ItaniumMangle.cpp:1774).
- **Found via:** corpus/runs/simdjson (wave 2; dedup key
  `mangle-reflection-deduction-guide-function-encoding-unreachable`). Exposed by a run-side
  leak (a `namespace sd = simdjson;` alias inside the reflected fixture namespace pulled
  the whole library into the walk — itself fixed by BINDER-0028) but real independently.
- **Fix:** Declaration-kind guides encode like Template-kind ones — `"dg"` + deduced
  template + ODR-hash discriminator — with the specialization's own function type folded
  in (TC-0009's lesson: `AddFunctionDecl` no-ops in specialization context, so the type is
  what separates `Box<int>`'s guide from `Box<double>`'s).
- **Repro:** `repros/TC-0015/repro.cpp` (14 lines; also
  `corpus/runs/simdjson/findings_draft/ice_repro.cpp` for the field shape); regression test
  `deduction-guide-spec-reflection-mangling.pass.cpp` (manglability + no linker folding of
  distinct specs). Category **D** (reflection-NTTP mangling).
