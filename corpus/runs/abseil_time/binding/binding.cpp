// Single-stage bindings for Abseil time types: absl::Duration + absl::Time bound directly (their
// arithmetic/comparison operators -> dunders, operator<< -> __str__), with a timetest fixture
// namespace supplying the factory/conversion free functions needed to construct/extract values
// (Duration/Time have no public integer constructors). Links the prebuilt absl static lib.
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(abseil_time_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
