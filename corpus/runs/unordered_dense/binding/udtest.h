// Fixture namespace for the unordered_dense run: VALUE-SETTING population only.
// The query surface (contains/count/at/erase/operator[]/size/empty/...) binds
// head-on off the real ankerl::unordered_dense::map/set specs. What Python
// cannot express is insert()'s pair<iterator, bool> return (iterators have no
// Python value semantics) and assignment through __getitem__ -- so put()/add()
// wrap the canonical C++ insertion idioms. Shared by binding/binding.cpp and
// tests/oracle_native.cpp.
#pragma once
#include <cstdint>
#include <string>
#include <string_view>

#include <ankerl/unordered_dense.h>

namespace udtest {

inline void put(ankerl::unordered_dense::map<int, std::string>& m, int k,
                std::string_view v) {
    m[k] = std::string(v);
}
inline void add(ankerl::unordered_dense::set<int>& s, int v) { s.insert(v); }

}  // namespace udtest
