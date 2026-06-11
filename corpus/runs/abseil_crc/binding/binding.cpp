// Binding TU for the Abseil CRC32C run: absl::crc32c_t bound head-on (ctors,
// explicit operator uint32_t -> __int__, namespace operator<< -> __str__), plus
// the crctest fixture namespace exposing absl's free CRC computation/arithmetic
// functions (see crctest.h).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string_view.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;

NB_MODULE(abseil_crc_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
