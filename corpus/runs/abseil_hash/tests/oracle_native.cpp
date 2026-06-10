// Native C++ ground-truth oracle for the Abseil hash-container binding (Layer-1
// differential). Drives the SAME surface the Python test drives through the
// binding (hmtest population + the head-on query/mutation API) and emits JSON.
#include "../binding/hmtest.h"

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

    using namespace hmtest;

    // --- flat_hash_map<int, std::string> ---
    absl::flat_hash_map<int, std::string> d;
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
    d.clear();
    add_b("m_cleared_empty", d.empty());

    // Capacity growth across many inserts is implementation-deterministic for a
    // shared abseil build: same keys -> same growth.
    absl::flat_hash_map<int, std::string> g;
    for (int i = 0; i < 100; ++i) put(g, i, "v");
    add_i("m_grown_size", (long long) g.size());
    add_i("m_grown_capacity", (long long) g.capacity());
    add_i("m_bucket_count", (long long) g.bucket_count());

    // --- flat_hash_set<int> ---
    absl::flat_hash_set<int> s;
    add(s, 7);
    add(s, 7);
    add(s, 8);
    add_i("s_size", (long long) s.size());
    add_b("s_has7", s.contains(7));
    add_b("s_has9", s.contains(9));
    add_i("s_erase7", (long long) s.erase(7));
    add_i("s_size_after", (long long) s.size());

    // --- node_hash_map<int, std::string> (pointer-stable sibling) ---
    absl::node_hash_map<int, std::string> n;
    put_node(n, 5, "five");
    put_node(n, 6, "six");
    add_i("n_size", (long long) n.size());
    add_b("n_has5", n.contains(5));
    add_s("n_at5", n.at(5));
    add_i("n_erase5", (long long) n.erase(5));
    add_s("n_idx6", n[6]);

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
