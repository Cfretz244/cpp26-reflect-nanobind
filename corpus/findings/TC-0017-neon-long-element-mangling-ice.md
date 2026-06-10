# TC-0017 — mangling 64-bit NEON vector specializations ICE'd on LP64 ("unexpected Neon vector element type")

- **Status:** FIXED locally (`clang/lib/AST/ItaniumMangle.cpp`, `mangleNeonVectorType`);
  upstream filing this wave (note: likely reproducible in UPSTREAM clang on Darwin too —
  the reflection walk merely makes it easy to reach).
- **Found via:** DRIVER, writing TC-0016's regression test: `is_class_type` over
  `members_of(^^::)` reaches the predeclared NEON typedefs (`__Int64x1_t` et al.); the
  `<meta>` builtin-expansion workaround substitutes a specialization over the vector type,
  and codegen-mode eager mangling hit `llvm_unreachable("unexpected Neon vector element
  type")` — on LP64 Darwin the 64-bit NEON element type is plain `long`/`unsigned long`,
  and the switches only handled `LongLong`/`ULongLong`. `-fsyntax-only` clean; `-c` ICEs.
- **Fix:** added `Long`/`ULong` arms (`int64_t`/`uint64_t`, and `poly64_t` in the
  polynomial switch).
- **Repro:** `repros/TC-0017/repro.cpp` (`__Int64`-filtered `is_class_type` walk, compile
  with `-c`). Regression coverage: TC-0016's test exercises the same walk in codegen mode.
  Category **D** (mangling robustness).
