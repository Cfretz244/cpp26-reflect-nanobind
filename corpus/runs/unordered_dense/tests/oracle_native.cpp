// Native C++ ground-truth oracle for the ankerl::unordered_dense binding
// (Layer-1 differential). Drives the SAME surface the Python test drives
// through the binding (udtest population + the head-on query/mutation API) and
// emits ONE JSON object on stdout.
#include "../binding/udtest.h"

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

    using namespace udtest;

    // --- map<int, std::string> ---
    ankerl::unordered_dense::map<int, std::string> d;
    put(d, 1, "one");
    put(d, 2, "two");
    add_i("m_size", (long long) d.size());
    add_b("m_has1", d.contains(1));
    add_b("m_has3", d.contains(3));
    add_s("m_at1", d.at(1));
    add_s("m_idx2", d[2]);                       // operator[] read (present key)
    add_i("m_count3", (long long) d.count(3));
    add_s("m_idx9_default", d[9]);               // operator[] default-INSERTS
    add_i("m_size_after_idx9", (long long) d.size());
    add_i("m_erase1", (long long) d.erase(1));
    add_i("m_erase1_again", (long long) d.erase(1));
    add_i("m_size_after_erase", (long long) d.size());
    put(d, 2, "TWO");                            // overwrite via put
    add_s("m_at2_overwritten", d.at(2));
    add_b("m_empty_before_clear", d.empty());
    d.clear();
    add_b("m_cleared_empty", d.empty());

    // Capacity growth across many inserts is deterministic for a fixed header
    // build: same keys -> same growth.
    ankerl::unordered_dense::map<int, std::string> g;
    for (int i = 0; i < 100; ++i) put(g, i, "v");
    add_i("m_grown_size", (long long) g.size());
    add_i("m_grown_bucket_count", (long long) g.bucket_count());

    // --- set<int> ---
    ankerl::unordered_dense::set<int> s;
    add(s, 7);
    add(s, 7);                                   // duplicate: no-op
    add(s, 8);
    add_i("s_size", (long long) s.size());
    add_b("s_has7", s.contains(7));
    add_b("s_has9", s.contains(9));
    add_i("s_count7", (long long) s.count(7));
    add_i("s_erase7", (long long) s.erase(7));
    add_i("s_erase7_again", (long long) s.erase(7));
    add_i("s_size_after", (long long) s.size());
    add_b("s_empty_after_clear", ([&]{ s.clear(); return s.empty(); })());

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
