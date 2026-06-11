# TC-0018: 2^15-element NTTP packs miscompile ("excess elements in array initializer")

- **Status**: fixed locally (llvm-project submodule); upstream filing pending.
- **Component**: clang AST pack-substitution storage. ROOT CAUSE (corrected
  during the fix; the original write-up blamed PackIndex alone):
  `SubstNonTypeTemplateParmPackExpr::NumArguments : 15`
  (`clang/include/clang/AST/ExprCXX.h`) -- the count of template arguments
  the pack was instantiated with. At 32768 elements it wraps to 0 and the
  pack expands empty/wrapped, producing the bogus diagnostic. The
  same-family narrow fields are also handled:
  - `SubstNonTypeTemplateParmExpr::{Index:15, PackIndex:15, Final:1}` --
    PackIndex (index+1 encoding) now stored full-width (packs into the same
    object size);
  - `SubstTemplateTypeParmTypeBitfields::PackIndex` 15 -> 26 bits (fills the
    64-bit word) + ctor assert;
  - `UncommonTemplateNameStorage::Bits.Data:15` (pack index or stored-arg
    count for template template packs) -- base-ctor assert so a wrap is a
    hard assert, not a silent miscompile.
- **Found by**: the emit backend's first full-fixture TU (~50KB) failing to
  lift to static storage; minimized to a `define_static_string` length probe.
  NOT reflection-specific: a plain C++ 32768-element char pack
  (`arr<((void)Is, 'x')...>` from `make_integer_sequence<unsigned, 32768>`)
  fails identically, so this reproduces in UPSTREAM clang too --
  reflection's `define_static_string` just makes >=32K packs trivial to form.
- **Symptom**: `std::define_static_string(s)` for `s.size() >= 32768` fails
  with `excess elements in array initializer` out of
  `__define_static::FixedArray` (32767 works). No implementation-limit
  diagnostic; counts that wrap to nonzero would silently mis-size.
- **Repro**: `repros/TC-0018/repro.cpp` (+ the plain-C++ no-reflection probe
  embedded in the finding); regression test
  `llvm-project/libcxx/test/std/experimental/reflection/define-static-string-large.pass.cpp`
  (32767 / 32768 / 50000 round-trips).
- **Binder impact**: none on the constexpr path; the emit backend chunks all
  generated text at 8K (`emit_chunk_size`) so it works on unpatched
  toolchains regardless.
- **Category**: E (constant-evaluator robustness) for tracking issue #308.
