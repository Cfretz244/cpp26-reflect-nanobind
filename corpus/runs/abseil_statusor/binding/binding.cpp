// Binding TU for the Abseil StatusOr run: three real StatusOr<T> specializations
// bound head-on (ok/status/value), Status + StatusCode bound in-module (status()
// returns const Status&), and the sotest factory fixtures (see sotest.h).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/optional.h>

#include "sotest.h"

namespace nb = nanobind;

NB_MODULE(abseil_statusor_ext, m) {
    nb::reflect_<^^absl::StatusOr<int>, ^^absl::StatusOr<std::string>,
                 ^^absl::StatusOr<double>, ^^absl::Status, ^^absl::StatusCode,
                 ^^sotest>(m);
}
