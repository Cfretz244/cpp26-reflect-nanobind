// Native C++ ground-truth oracle for the fmt binding (Layer-1 differential). It calls the SAME
// fmttest wrapper functions the Python test drives through the binding, and reads the SAME real
// fmt enum values, then emits them as a JSON object on stdout. run_gates captures that as
// expected.json; test_bindings.py asserts the bound module reproduces every value. Native harness
// and bindings share one compiler + one fmt, so any divergence is attributable to the binding
// layer. (Includes the shared fixtures so the expressions are byte-for-byte identical.)
#include "../binding/fmttest.h"
#include <fmt/color.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

// Minimal JSON string escaping (only \ and " can appear); values here are plain ASCII.
static std::string jstr(const std::string& s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '\\' || c == '"') o += '\\';
        o += c;
    }
    return o + "\"";
}

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) { kv.emplace_back(k, jstr(v)); };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };

    // free-function format wrappers
    add_s("apply_int_05d",  fmttest::apply_int("{:05d}", 42));
    add_s("apply_int_hex",  fmttest::apply_int("{:x}", 255));
    add_s("apply_double_f", fmttest::apply_double("{:.3f}", 3.14159));
    add_s("apply_str_r6",   fmttest::apply_str("{:>6}", "hi"));
    add_s("apply_two",      fmttest::apply_two("{0}-{1}-{0}", 7, 9));
    add_s("pad_left_7_5",   fmttest::pad_left(7, 5));
    add_s("hex_upper_255",  fmttest::hex_upper(255));
    add_s("join_ints",      fmttest::join_ints({1, 2, 3}, ", "));
    add_s("join_strs",      fmttest::join_strs({"a", "b", "c"}, "/"));

    // Formatter fixture (member-fn-template gap: apply_int binds, apply<T> is skipped)
    add_s("fmtr_bin",       fmttest::Formatter("{:08b}").apply_int(5));
    add_s("fmtr_spec",      fmttest::Formatter().spec());
    add_s("fmtr_hash_hex",  fmttest::Formatter("{:#x}").apply_int(255));

    // real fmt FORMATTING API, bound directly from fmt's own template instantiations / class
    add_s("ts_int_42",    fmt::to_string(42));
    add_s("ts_int_neg",   fmt::to_string(-7));
    add_s("ts_double_2_5",fmt::to_string(2.5));
    add_s("ts_ll_big",    fmt::to_string(9999999999LL));
    add_s("fi_123456",    fmt::format_int(123456).str());
    add_s("fi_neg",       fmt::format_int(-42).str());
    add_i("fi_size",      static_cast<std::int64_t>(fmt::format_int(123456).size()));

    // real fmt enum values (Layer-1 ground truth = the library's fixed enumerator values)
    add_i("color_red",       static_cast<std::int64_t>(fmt::color::red));
    add_i("color_alice_blue",static_cast<std::int64_t>(fmt::color::alice_blue));
    add_i("term_red",        static_cast<std::int64_t>(fmt::terminal_color::red));
    add_i("emph_bold",       static_cast<std::int64_t>(fmt::emphasis::bold));
    add_i("emph_italic",     static_cast<std::int64_t>(fmt::emphasis::italic));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << jstr(kv[i].first) << ":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
