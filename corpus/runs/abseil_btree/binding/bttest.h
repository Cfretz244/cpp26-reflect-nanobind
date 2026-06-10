// Fixture namespace for the Abseil btree run: VALUE-SETTING population only
// (same rationale as abseil_hash's hmtest -- insert() returns
// pair<iterator,bool>, which has no Python representation). first_key exposes
// the ordered-ness through the C++ iteration API the binder cannot surface yet.
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <string>
#include <string_view>

#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"

namespace bttest {

inline void put(absl::btree_map<int, std::string>& m, int k, std::string_view v) {
    m[k] = std::string(v);
}
inline void add(absl::btree_set<int>& s, int v) { s.insert(v); }

// Ordered-structure observations (begin() is C++-only; these pin the ORDER the
// Python-side tests then verify differentially).
inline int first_key(const absl::btree_map<int, std::string>& m) {
    return m.begin()->first;
}
inline int first_elem(const absl::btree_set<int>& s) { return *s.begin(); }

}  // namespace bttest
