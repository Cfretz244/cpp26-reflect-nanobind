// TC-0019: symbol_of(op_caret_equals) returns "^" instead of "^=".
//
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fsyntax-only repro.cpp
//
// Root cause: a typo in BOTH operator-symbol tables in libcxx's <meta>
// (symbol_of and u8symbol_of): the entry between "%=" and "&=" reads "^"
// (duplicating the plain-xor entry) instead of "^=". Every other compound
// assignment is correct. Field shape: the emit backend rendered
// absl::int128's operator^= binding as `self.operator^(...)` -- a
// non-existent member -- failing the production compile (the __ixor__
// dunder, from the binder's own operator_dunder map, was correct).
#include <meta>
#include <string_view>

static_assert(std::meta::symbol_of(std::meta::operators::op_caret_equals)
              == std::string_view("^="));
static_assert(std::meta::u8symbol_of(std::meta::operators::op_caret_equals)
              == std::u8string_view(u8"^="));
int main() {}
