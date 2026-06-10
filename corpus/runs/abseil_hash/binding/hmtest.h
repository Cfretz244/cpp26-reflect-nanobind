// Fixture namespace for the Abseil hash-container run: VALUE-SETTING population
// only. The query surface (contains/count/find/erase/at/operator[]/size/...)
// binds head-on; what Python cannot express is insert()'s pair<iterator, bool>
// return (iterators have no Python value semantics) and assignment through
// __getitem__ -- so put()/add() wrap the canonical C++ insertion idioms.
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"

namespace hmtest {

inline void put(absl::flat_hash_map<int, std::string>& m, int k,
                std::string_view v) {
    m[k] = std::string(v);
}
inline void add(absl::flat_hash_set<int>& s, int v) { s.insert(v); }
inline void put_node(absl::node_hash_map<int, std::string>& m, int k,
                     std::string_view v) {
    m[k] = std::string(v);
}

}  // namespace hmtest
