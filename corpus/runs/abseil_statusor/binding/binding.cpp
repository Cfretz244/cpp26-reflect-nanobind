// Single-stage bindings for the Abseil StatusOr run: three real StatusOr<T>
// specializations bound head-on (ok/status/value), Status + StatusCode bound
// in-module (status() returns const Status&), and the sotest factory fixtures
// (see sotest.h). value() is the entity-proxy showcase -- re-exported from a
// PRIVATE base via using-declaration (TC-0003, -fentity-proxy-reflection).
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/optional.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(abseil_statusor_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
