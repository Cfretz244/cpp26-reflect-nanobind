// Single-stage bindings for the Abseil hash-container run: flat_hash_map/
// flat_hash_set/node_hash_map bound head-on (the BINDER-0008 payoff --
// reachability discovery keeps the policy internals out; default-instantiable
// member templates supply the heterogeneous query surface incl.
// operator[] -> __getitem__), plus the hmtest value-setting population fixtures.
// The bind set lives in binding_args.h (shared with the emit generator).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/pair.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;

NB_MODULE(abseil_hash_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
