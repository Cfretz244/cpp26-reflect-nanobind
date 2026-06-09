// Native C++ ground-truth oracle for the Abseil int128/uint128 binding (Layer-1 differential).
// Computes the SAME arithmetic the Python test drives through the binding, using absl::int128/
// uint128 directly, and emits each result as a decimal string (the same channel the bound module
// reads via __str__). Shared compiler + shared Abseil => any divergence is the binding layer's.
#include "absl/numeric/int128.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static std::string s128(absl::int128 v) { std::ostringstream o; o << v; return o.str(); }
static std::string su128(absl::uint128 v) { std::ostringstream o; o << v; return o.str(); }

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };

    absl::int128 a = 1000000, b = 7;
    add("a", s128(a));
    add("mul", s128(a * b));
    add("add", s128(a + b));
    add("sub", s128(a - b));
    add("div", s128(a / b));
    add("mod", s128(a % b));
    add("shl", s128(a << 10));
    add("neg", s128(-a));
    add("eq", a == absl::int128(1000000) ? "true" : "false");
    add("lt", b < a ? "true" : "false");

    // 128-bit width: products/sums that overflow 64 bits.
    absl::uint128 big = absl::Uint128Max();           // 2^128 - 1
    add("u_max", su128(big));
    add("u_max_plus1", su128(big + 1));               // wraps to 0
    absl::uint128 p = absl::uint128(1) << 100;        // 2^100
    add("u_2pow100", su128(p));
    add("u_2pow100_x8", su128(p * 8));                // 2^103

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
