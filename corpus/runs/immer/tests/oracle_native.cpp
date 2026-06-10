// Native C++ ground-truth oracle for the immer binding (Layer-1 differential).
//
// Drives the EXACT chain of persistent updates the Python test drives through the bound
// module -- same push_back/set/take sequence on vector, same set/insert/erase sequence on
// map and set -- and emits every observable (the contents of EVERY intermediate version,
// plus size/count/find/at error surfaces) as one JSON object on stdout. Shared compiler +
// shared header-only immer => any divergence is the binding layer's. The heart of the
// differential is immer's value semantics with structural sharing: each mutating call
// RETURNS A NEW container leaving the receiver untouched -- the oracle asserts both the new
// version's contents AND the original's after each step.
#include <immer/vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::pair<std::string, std::string>> kv;

void add_raw(const char* k, const std::string& json) { kv.emplace_back(k, json); }
void add_i(const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); }
void add_b(const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); }

std::string esc(const std::string& v) {
    std::string e = "\"";
    for (char c : v) {
        if (c == '\\' || c == '"') { e += '\\'; e += c; }
        else if (c == '\n') e += "\\n";
        else e += c;
    }
    return e + "\"";
}
void add_s(const char* k, const std::string& v) { kv.emplace_back(k, esc(v)); }

// contents of an int vector as a JSON array
template <typename V>
std::string ivec_json(const V& v) {
    std::ostringstream o; o << "[";
    for (std::size_t i = 0; i < v.size(); ++i) { if (i) o << ","; o << v[i]; }
    o << "]";
    return o.str();
}
template <typename V>
std::string svec_json(const V& v) {
    std::ostringstream o; o << "[";
    for (std::size_t i = 0; i < v.size(); ++i) { if (i) o << ","; o << esc(v[i]); }
    o << "]";
    return o.str();
}

}  // namespace

int main() {
    using ivec = immer::vector<int>;
    using svec = immer::vector<std::string>;
    using imap = immer::map<int, int>;
    using iset = immer::set<int>;

    // ---------- vector<int>: persistent push_back / set / take chain ----------
    ivec v0;
    ivec v1 = v0.push_back(10);
    ivec v2 = v1.push_back(20);
    ivec v3 = v2.push_back(30);
    ivec v3b = v3.set(1, 99);        // new version with index 1 changed
    ivec v3t = v3.take(2);           // prefix of length 2

    add_raw("v0", ivec_json(v0));
    add_raw("v1", ivec_json(v1));
    add_raw("v2", ivec_json(v2));
    add_raw("v3", ivec_json(v3));    // ORIGINAL after set/take: must be unchanged [10,20,30]
    add_raw("v3b", ivec_json(v3b));
    add_raw("v3t", ivec_json(v3t));
    add_i("v3_size", static_cast<std::int64_t>(v3.size()));
    add_b("v0_empty", v0.empty());
    add_b("v3_empty", v3.empty());
    add_i("v3_front", v3.front());
    add_i("v3_back", v3.back());
    add_i("v3_at1", v3.at(1));
    add_b("v2_eq_take2_of_v3", (v2 == v3.take(2)));   // value equality across versions
    add_b("v2_eq_v3", (v2 == v3));

    // ---------- vector<string> ----------
    svec s0;
    svec s1 = s0.push_back(std::string("alpha"));
    svec s2 = s1.push_back(std::string("beta"));
    svec s2b = s2.set(0, std::string("ALPHA"));
    add_raw("s2", svec_json(s2));     // original unchanged after set
    add_raw("s2b", svec_json(s2b));
    add_s("s2_front", s2.front());
    add_s("s2_back", s2.back());

    // ---------- map<int,int>: persistent set / erase chain ----------
    imap m0;
    imap m1 = m0.set(1, 100);
    imap m2 = m1.set(2, 200);
    imap m3 = m2.set(1, 111);          // overwrite key 1 -> new version
    imap m3e = m3.erase(2);            // erase key 2 -> new version

    add_i("m0_size", static_cast<std::int64_t>(m0.size()));
    add_i("m2_size", static_cast<std::int64_t>(m2.size()));
    add_i("m3_size", static_cast<std::int64_t>(m3.size()));
    add_i("m3e_size", static_cast<std::int64_t>(m3e.size()));
    add_i("m2_at1", m2[1]);            // original keeps 100 after m3 overwrote to 111
    add_i("m3_at1", m3[1]);            // 111
    add_i("m2_count2", static_cast<std::int64_t>(m2.count(2)));
    add_i("m3e_count2", static_cast<std::int64_t>(m3e.count(2)));
    add_i("m2_get_missing", m2[99]);   // default-constructed value (0) for absent key
    // find returns const T* -> bound dereferenced (value) / None when null
    const int* fp = m2.find(2);
    add_i("m2_find2", fp ? *fp : -1);
    add_b("m2_find99_null", m2.find(99) == nullptr);
    // insert(value_type) overload (pair)
    imap m4 = m3e.insert(std::make_pair(5, 500));
    add_i("m4_at5", m4[5]);
    add_i("m3e_count5", static_cast<std::int64_t>(m3e.count(5)));  // original lacks 5

    // ---------- set<int>: persistent insert / erase chain ----------
    iset t0;
    iset t1 = t0.insert(7);
    iset t2 = t1.insert(8);
    iset t3 = t2.insert(7);            // duplicate insert -> no growth, new version equal
    iset t3e = t3.erase(8);

    add_i("t0_size", static_cast<std::int64_t>(t0.size()));
    add_i("t2_size", static_cast<std::int64_t>(t2.size()));
    add_i("t3_size", static_cast<std::int64_t>(t3.size()));   // still 2 (7 already present)
    add_i("t3e_size", static_cast<std::int64_t>(t3e.size()));
    add_i("t2_count7", static_cast<std::int64_t>(t2.count(7)));
    add_i("t2_count9", static_cast<std::int64_t>(t2.count(9)));
    add_b("t2_find8_present", t2.find(8) != nullptr);
    add_b("t2_find9_null", t2.find(9) == nullptr);
    add_i("t3e_count8", static_cast<std::int64_t>(t3e.count(8)));  // erased
    add_i("t2_count8_after_erase", static_cast<std::int64_t>(t2.count(8)));  // orig keeps 8

    std::cout << "{";
    for (std::size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
