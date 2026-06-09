// Single-stage bindings for Abseil's 128-bit integers (absl::int128 / absl::uint128).
//
// These are concrete value types whose arithmetic/bitwise/comparison operators (member + free)
// the binder maps to Python dunders directly, and whose free operator<<(std::ostream&, ...) is
// surfaced as __str__ (BINDER-0007). No fixture wrappers — the real types are bound head-on.
// Links the prebuilt absl static lib (int128 division/streaming live in int128.cc).
#include <nanobind/nb_reflect.h>

#include "absl/numeric/int128.h"

namespace nb = nanobind;

NB_MODULE(abseil_numeric_ext, m) {
    nb::reflect_<^^absl::int128, ^^absl::uint128>(m);
}
