# TC-0019: symbol_of / u8symbol_of return "^" for op_caret_equals

- **Status**: fixed locally (libcxx <meta> header, both the llvm-project
  submodule source and the installed toolchain copy -- header-only, no
  compiler rebuild); upstream filing pending.
- **Component**: libcxx `include/meta`, the `op_names` tables in `symbol_of`
  and `u8symbol_of`: the entry between `"%="` and `"&="` reads `"^"`
  (duplicating plain xor) instead of `"^="`. All other compound assignments
  are correct.
- **Found by**: the abseil_numeric emit lane -- the generated TU spelled
  int128's `operator^=` binding as `self.operator^(...)` (no such member;
  B.emit_compile under Apple Clang). The constexpr lane never consults
  symbol_of (it splices `[:fn:]`), so the bug was invisible until a text
  backend rendered operator names.
- **Repro**: `repros/TC-0019/repro.cpp` (two static_asserts).
- **Regression test**: `llvm-project/libcxx/test/std/experimental/reflection/
  operator-symbol-tables.pass.cpp` (asserts the full compound-assignment row
  in both tables).
- **Category**: B-adjacent (header metafunction data, not Sema) -- file under
  a suitable category on tracking issue #308 when upstreamed.
