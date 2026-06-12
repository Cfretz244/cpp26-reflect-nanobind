// Single-stage bindings for Abseil's 128-bit integers (absl::int128 / absl::uint128).
//
// These are concrete value types whose arithmetic/bitwise/comparison operators (member + free)
// the binder maps to Python dunders directly, and whose free operator<<(std::ostream&, ...) is
// surfaced as __str__ (BINDER-0007). No fixture wrappers — the real types are bound head-on.
// Links the prebuilt absl static lib (int128 division/streaming live in int128.cc).
#include <mirrorbind/reflect.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(abseil_numeric_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
