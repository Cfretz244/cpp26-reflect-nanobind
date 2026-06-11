// TC-0018: NTTP packs of >= 32768 elements miscompile: the pack's stored
// argument count (SubstNonTypeTemplateParmPackExpr::NumArguments : 15) wraps,
// and the expansion produces a bogus "excess elements in array initializer"
// from __define_static::FixedArray. 32767 works; 32768 fails. Reproduces in
// plain C++ without reflection (see the finding write-up) -- reflection's
// define_static_string just makes such packs trivial to form.
//
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fconstexpr-steps=268435456 \
//     -fsyntax-only repro.cpp
//
// Expected: compiles. Pre-fix: "excess elements in array initializer".
#include <meta>
#include <string>

consteval const char* big(std::size_t n) {
    std::string s(n, 'x');
    return std::define_static_string(s);
}

constexpr const char* ok = big(32767);    // compiles
constexpr const char* bad = big(32768);   // excess elements in array initializer

int main() { return (ok[0] == 'x' && bad[0] == 'x') ? 0 : 1; }
