// Binding TU for the Abseil btree run: btree_map/btree_set bound head-on (the
// ordered half of BINDER-0008), plus bttest population/order fixtures.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/pair.h>

#include "bttest.h"

namespace nb = nanobind;

NB_MODULE(abseil_btree_ext, m) {
    nb::reflect_<^^absl::btree_map<int, std::string>,
                 ^^absl::btree_set<int>,
                 ^^bttest>(m);
}
