// Binding TU for the Abseil hash-container run: flat_hash_map/flat_hash_set/
// node_hash_map bound head-on (the BINDER-0008 payoff -- reachability discovery
// keeps the policy internals out; default-instantiable member templates supply
// the heterogeneous query surface incl. operator[] -> __getitem__), plus the
// hmtest value-setting population fixtures.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/pair.h>

#include "hmtest.h"

namespace nb = nanobind;

NB_MODULE(abseil_hash_ext, m) {
    nb::reflect_<^^absl::flat_hash_map<int, std::string>,
                 ^^absl::flat_hash_set<int>,
                 ^^absl::node_hash_map<int, std::string>,
                 ^^hmtest>(m);
}
