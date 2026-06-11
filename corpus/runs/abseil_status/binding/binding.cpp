// Single-stage bindings for absl::Status + absl::StatusCode (Abseil's canonical error type).
// Bound head-on (no wrappers). The std::string_view / std::optional casters are included for
// message() (-> string_view) and the payload API (-> optional<Cord>). Links the prebuilt absl lib.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/optional.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;

NB_MODULE(abseil_status_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
