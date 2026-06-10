// Native C++ ground-truth oracle for the Abseil btree binding (Layer-1
// differential). Drives the SAME surface the Python test drives through the
// binding and emits JSON; the ordered-structure observations (first_key/
// first_elem) come from the shared bttest fixtures.
#include "../binding/bttest.h"

#include <iostream>
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

    using namespace bttest;

    // --- btree_map<int, std::string> ---
    absl::btree_map<int, std::string> d;
    put(d, 30, "thirty");
    put(d, 10, "ten");
    put(d, 20, "twenty");
    add_i("m_size", (long long) d.size());
    add_b("m_has10", d.contains(10));
    add_b("m_has99", d.contains(99));
    add_s("m_at10", d.at(10));
    add_s("m_idx20", d[20]);
    add_i("m_first_key", first_key(d));            // ordered: 10 despite insert order
    add_s("m_idx5_default", d[5]);                 // default-inserts
    add_i("m_first_key_after5", first_key(d));     // ordered: now 5
    add_i("m_erase10", (long long) d.erase(10));
    add_i("m_size_after", (long long) d.size());
    add_i("m_count20", (long long) d.count(20));
    d.clear();
    add_b("m_cleared_empty", d.empty());

    // --- btree_set<int> ---
    absl::btree_set<int> s;
    add(s, 42);
    add(s, 7);
    add(s, 42);                                    // duplicate: no-op
    add(s, 19);
    add_i("s_size", (long long) s.size());
    add_i("s_first", first_elem(s));               // ordered: 7
    add_b("s_has42", s.contains(42));
    add_b("s_has8", s.contains(8));
    add_i("s_erase7", (long long) s.erase(7));
    add_i("s_first_after", first_elem(s));         // ordered: 19
    add_i("s_size_after", (long long) s.size());

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
