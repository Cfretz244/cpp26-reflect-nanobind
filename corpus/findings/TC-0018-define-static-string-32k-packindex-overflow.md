# TC-0018: define_static_string miscompiles at >= 32768 chars (15-bit PackIndex overflow)

- **Status**: found (fix not yet landed; emitter sidesteps via chunking)
- **Component**: clang-p2996 AST (`SubstNonTypeTemplateParmExpr::PackIndex : 15`,
  `clang/include/clang/AST/ExprCXX.h`; same field on `SubstTemplateTypeParmTypeBitfields`,
  `clang/include/clang/AST/Type.h`)
- **Found by**: the emit backend's first full-fixture TU (~50KB) failing to lift to
  static storage; minimized to a two-line `define_static_string` length probe.
- **Symptom**: `std::define_static_string(s)` for `s.size() >= 32768` fails with
  `excess elements in array initializer` out of `__define_static::FixedArray`
  (32767 works). No implementation-limit diagnostic; the failure mode is a bogus
  error from a wrapped 15-bit per-element PackIndex (stored as index+1), i.e. the
  same shape could silently miscompile other >=32K NTTP packs.
- **Repro**: `repros/TC-0018/repro.cpp`
- **Binder impact**: none on the constexpr path (strings are small);
  the emit backend (nb_reflect_emit.h) lifts all generated text in 16K chunks
  (`make_static_chunks`) and returns the TU as a chunk sequence, so it works on
  unpatched toolchains -- and giant single packs would be a compile-time problem
  anyway.
- **Fix direction**: widen PackIndex (and audit the sibling 15/16-bit pack
  fields) or diagnose the limit properly; note the bitfields exist in UPSTREAM
  clang too -- p2996's `reflect_constant_string` just makes 32K+ packs trivial
  to form, so this may be worth raising upstream-of-upstream.
- **Category**: E (constant-evaluator robustness) for tracking issue #308.
