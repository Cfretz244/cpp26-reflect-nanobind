// TC-0018: define_static_string / reflect_constant_string miscompiles for
// inputs >= 32768 characters.
//
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fconstexpr-steps=200000000 \
//     -fsyntax-only repro.cpp
//
// Expected: compiles (or a pointed implementation-limit diagnostic).
// Actual:   "error: excess elements in array initializer" out of
//           __define_static::FixedArray -- at 32767 chars it compiles, at
//           32768 it does not.
//
// Root cause: reflect_constant_string spells the whole string as ONE pack of
// char NTTPs; substituting it creates a SubstNonTypeTemplateParmExpr per
// element, whose PackIndex is a 15-bit bitfield (clang/AST/ExprCXX.h, stored
// as index+1) that overflows at element 32767 -- the FixedArray
// specialization's deduced bound then disagrees with its initializer list.
// The same 15-bit PackIndex exists on SubstTemplateTypeParmType
// (clang/AST/Type.h) for TYPE packs. Field shape: any generated-source
// emitter lifting >32K of text via define_static_string (the nanobind emit
// backend's first full TU was 50K).
#include <meta>
#include <string>

consteval const char* big(std::size_t n) {
    std::string s(n, 'x');
    return std::define_static_string(s);
}

constexpr const char* ok = big(32767);    // compiles
constexpr const char* bad = big(32768);   // excess elements in array initializer

int main() { return (ok[0] == 'x' && bad[0] == 'x') ? 0 : 1; }
