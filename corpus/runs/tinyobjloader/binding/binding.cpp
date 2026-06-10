// Single-stage bindings for tinyobjloader v1.0.7 (Tier 2: POD aggregates + nested
// vectors). The library's own concrete types are bound head-on via reflection:
// tinyobj::index_t / mesh_t / shape_t / attrib_t / material_t. LoadObj is an out-param
// front, so the objtest::parse_obj fixture drives the std::istream* overload and returns
// an objtest::LoadResult aggregating those same real types (see objtest.h).
//
// This is the ONE TU that defines TINYOBJLOADER_IMPLEMENTATION (the single-header rule).
#define TINYOBJLOADER_IMPLEMENTATION
#include "objtest.h"

#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>

namespace nb = nanobind;

NB_MODULE(tinyobjloader_ext, m) {
    nb::reflect_<^^tinyobj::index_t,
                 ^^tinyobj::mesh_t,
                 ^^tinyobj::shape_t,
                 ^^tinyobj::attrib_t,
                 ^^tinyobj::material_t,
                 ^^objtest>(m);
}
