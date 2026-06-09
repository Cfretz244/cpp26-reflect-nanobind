// Single-stage bindings for Abseil time types: absl::Duration + absl::Time bound directly (their
// arithmetic/comparison operators -> dunders, operator<< -> __str__), with a timetest fixture
// namespace supplying the factory/conversion free functions needed to construct/extract values
// (Duration/Time have no public integer constructors). Links the prebuilt absl static lib.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>

#include "timetest.h"

namespace nb = nanobind;

NB_MODULE(abseil_time_ext, m) {
    nb::reflect_<^^absl::Duration, ^^absl::Time, ^^timetest>(m);
}
