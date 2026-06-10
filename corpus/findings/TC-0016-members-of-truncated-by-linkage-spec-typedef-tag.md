# TC-0016 — members_of silently truncated by the implicit tag of `extern "C" { typedef struct X X; }`

- **Status:** FIXED locally (`clang/lib/AST/ExprConstantMeta.cpp`, `findIterableMember`);
  upstream filing this wave. TC-0011's sibling (walk derailment), at TU/linkage-spec scope.
- **Found via:** DRIVER triage of corpus/runs/box2d (wave 2): after TC-0014 unblocked
  compilation, box2d's free operators (`operator==(const b2Vec2&, ...)` at global scope)
  silently never bound — `members_of(^^::)` stopped at `PyModuleDef` (1484 of the TU's
  members), exactly where Python.h's `pytypedefs.h` declares `typedef struct PyModuleDef
  PyModuleDef;` inside its `extern "C"` block. EVERY global decl after Python.h was
  invisible to reflection in every nanobind TU.
- **Root cause:** the implicit tag from a typedef in a linkage-spec block is semantically
  injected into the enclosing (file/namespace) scope but sits lexically in the block;
  stepping it via the semantic chain (`getPrevMultDCDeclInSemaContext`) walked BACKWARD out
  of the block and ended the enumeration.
- **Fix:** two halves — (1) stepping from a decl whose lexical context is a
  LinkageSpecDecl walks the LEXICAL block (mirroring TC-0011's rewrite; the pop-out resumes
  the enclosing scope), and (2) the namespace MultDC tail-hop skips linkage-spec-lexical
  decls (they are enumerated through the block; re-entering one cycled the walk forever —
  caught while writing the regression test's namespace case).
- **Repro:** `repros/TC-0016/repro.cpp` (one line of trigger); regression test
  `members-of-linkage-spec-typedef-tag.pass.cpp` (TU + namespace shapes). Category **C**
  (member/namespace enumeration).
