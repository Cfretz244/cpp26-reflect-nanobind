// objtest -- thin parse fixture for the tinyobjloader run, shared by binding.cpp and the
// native oracle.
//
// WHY a fixture: tinyobj::LoadObj is an out-param front -- it returns bool and fills
// attrib_t*/vector<shape_t>*/vector<material_t>* plus string* warn/err, taking the .obj
// either from a filename or a std::istream*. None of that shape is expressible to Python
// through the binder (out-params, raw istream, raw string-error channels). parse_obj()
// drives the std::istream* overload over an in-memory .obj string and returns a plain
// LoadResult struct AGGREGATING the library's OWN real types -- tinyobj::attrib_t, the
// real vector<shape_t>, vector<material_t>, and the bool/warn/err. The library's concrete
// types (attrib_t/shape_t/mesh_t/material_t/index_t) are what the tests actually read; the
// fixture only performs the out-param call Python cannot.
#pragma once

#include "tiny_obj_loader.h"

#include <sstream>
#include <string>
#include <vector>

namespace objtest {

// Aggregates the library's own real LoadObj outputs into one Python-returnable value.
struct LoadResult {
    bool success = false;
    std::string warn;
    std::string err;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

// Parse a .obj (no .mtl resolution) from an in-memory string. triangulate as requested.
inline LoadResult parse_obj(const std::string& obj_text, bool triangulate) {
    LoadResult r;
    std::istringstream iss(obj_text);
    // readMatFn = nullptr: no .mtl file lookup (in-memory only), matches the oracle.
    r.success = tinyobj::LoadObj(&r.attrib, &r.shapes, &r.materials, &r.warn, &r.err,
                                 &iss, /*readMatFn=*/nullptr, triangulate,
                                 /*default_vcols_fallback=*/true);
    return r;
}

}  // namespace objtest
