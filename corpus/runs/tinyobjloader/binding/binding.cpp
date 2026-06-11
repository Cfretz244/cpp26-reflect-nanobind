// Single-stage bindings for tinyobjloader v1.0.7 (constexpr lane). The
// library's own concrete types are bound head-on; objtest::parse_obj drives
// the out-param LoadObj front (see objtest.h). This TU defines
// TINYOBJLOADER_IMPLEMENTATION (the single-header rule).
#define TINYOBJLOADER_IMPLEMENTATION
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(tinyobjloader_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
