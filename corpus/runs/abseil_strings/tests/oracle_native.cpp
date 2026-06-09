// Native C++ ground-truth oracle for the Abseil strings binding (Layer-1 differential).
// Drives the SAME Cord surface + strtest fixtures the Python test drives through the
// binding, using native Abseil, and emits each result as JSON.
#include "../binding/strtest.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, long long v) {
        kv.emplace_back(k, std::to_string(v));
    };
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };
    auto add_b = [&](const char* k, bool v) {
        kv.emplace_back(k, v ? "true" : "false");
    };

    using namespace strtest;

    // --- Cord bound surface ---
    absl::Cord c(std::string_view("hello "));
    c.Append(std::string_view("world"));
    add_i("cord_size", static_cast<long long>(c.size()));
    add_s("cord_str", cord_str(c));
    { std::ostringstream o; o << c; add_s("cord_stream", o.str()); }  // __str__ channel

    c.Prepend(std::string_view(">> "));
    add_s("cord_prepended", cord_str(c));
    c.RemovePrefix(3);
    add_s("cord_rm_prefix", cord_str(c));
    c.RemoveSuffix(5);
    add_s("cord_rm_suffix", cord_str(c));

    absl::Cord full(std::string_view("hello world"));
    add_s("cord_subcord", cord_str(full.Subcord(6, 5)));
    add_b("cord_starts", full.StartsWith(std::string_view("hello")));
    add_b("cord_ends", full.EndsWith(std::string_view("world")));
    add_i("cord_cmp_eq", full.Compare(absl::Cord(std::string_view("hello world"))));
    add_i("cord_cmp_lt", absl::Cord(std::string_view("apple"))
                             .Compare(absl::Cord(std::string_view("banana"))));
    absl::Cord empty_after;
    empty_after.Append(std::string_view("x"));
    empty_after.Clear();
    add_b("cord_cleared_empty", empty_after.empty());

    // --- strtest fixtures over the free-function API ---
    add_s("cat2", cat2("foo", "bar"));
    add_s("cat3", cat3("a", "-", "b"));
    add_s("cat_int", cat_int("n=", 42));
    add_s("join", join({"a", "b", "c"}, "-"));
    {
        auto parts = split("x,y,z", ',');
        add_i("split_n", static_cast<long long>(parts.size()));
        add_s("split_1", parts[1]);
    }
    add_i("to_int_ok", *to_int("12345"));
    add_b("to_int_bad_is_none", !to_int("not a number").has_value());
    add_s("upper", upper("MixedCase42"));
    add_s("lower", lower("MixedCase42"));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
