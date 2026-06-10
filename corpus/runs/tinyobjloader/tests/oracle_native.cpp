// Native C++ ground-truth oracle for the tinyobjloader binding (Layer-1 differential).
// Parses the SAME in-memory .obj the Python test parses, through native tinyobj via the
// shared objtest::parse_obj fixture, and emits every observable (vertex/normal/texcoord
// counts + sample values, per-shape names, mesh face indices, num_face_vertices,
// material_ids, material name/illum) as one JSON object on stdout. Shared compiler +
// shared header-only tinyobj => any divergence is the binding layer's.
//
// This TU also defines TINYOBJLOADER_IMPLEMENTATION (the native build links no other TU).
#define TINYOBJLOADER_IMPLEMENTATION
#include "../binding/objtest.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// The shared .obj: 4 vertices, 2 triangles (one quad triangulated), normals + texcoords,
// two named shapes. Mirrored verbatim in test_bindings.py.
static const char* kObj = R"OBJ(
# tiny test mesh
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 0.0 1.0 0.0
vn 0.0 0.0 1.0
vt 0.0 0.0
vt 1.0 0.0
vt 1.0 1.0
vt 0.0 1.0
o first
f 1/1/1 2/2/1 3/3/1
o second
f 1/1/1 3/3/1 4/4/1
)OBJ";

static std::string esc(const std::string& v) {
    std::string e = "\"";
    for (char c : v) {
        if (c == '\\' || c == '"') { e += '\\'; e += c; }
        else if (c == '\n') e += "\\n";
        else e += c;
    }
    return e + "\"";
}

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto S = [&](const char* k, const std::string& v) { kv.emplace_back(k, esc(v)); };
    auto I = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto B = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto F = [&](const char* k, double v) {
        std::ostringstream o; o << v; kv.emplace_back(k, o.str());
    };

    objtest::LoadResult r = objtest::parse_obj(kObj, /*triangulate=*/true);

    B("success", r.success);
    // attrib: flat float arrays, 3 components per vertex/normal, 2 per texcoord.
    I("n_vertices_floats", static_cast<std::int64_t>(r.attrib.vertices.size()));
    I("n_normals_floats", static_cast<std::int64_t>(r.attrib.normals.size()));
    I("n_texcoords_floats", static_cast<std::int64_t>(r.attrib.texcoords.size()));
    F("v0_x", r.attrib.vertices.empty() ? 0.0 : r.attrib.vertices[0]);
    F("v2_x", r.attrib.vertices.size() > 6 ? r.attrib.vertices[6] : 0.0);  // 3rd vertex x = 1.0
    F("vn0_z", r.attrib.normals.size() > 2 ? r.attrib.normals[2] : 0.0);   // 1.0

    I("n_shapes", static_cast<std::int64_t>(r.shapes.size()));
    if (r.shapes.size() >= 2) {
        S("shape0_name", r.shapes[0].name);
        S("shape1_name", r.shapes[1].name);
        const auto& m0 = r.shapes[0].mesh;
        I("shape0_n_indices", static_cast<std::int64_t>(m0.indices.size()));
        I("shape0_n_faces", static_cast<std::int64_t>(m0.num_face_vertices.size()));
        I("shape0_face0_nverts", m0.num_face_vertices.empty()
              ? -1 : static_cast<std::int64_t>(m0.num_face_vertices[0]));
        // first face's three vertex indices (0-based into attrib.vertices/3)
        I("shape0_idx0_v", m0.indices.size() > 0 ? m0.indices[0].vertex_index : -1);
        I("shape0_idx1_v", m0.indices.size() > 1 ? m0.indices[1].vertex_index : -1);
        I("shape0_idx2_v", m0.indices.size() > 2 ? m0.indices[2].vertex_index : -1);
        I("shape0_idx0_vt", m0.indices.size() > 0 ? m0.indices[0].texcoord_index : -1);
        I("shape0_idx0_vn", m0.indices.size() > 0 ? m0.indices[0].normal_index : -1);
        // second shape, second face index (4th vertex = index 3)
        const auto& m1 = r.shapes[1].mesh;
        I("shape1_idx2_v", m1.indices.size() > 2 ? m1.indices[2].vertex_index : -1);
    }
    I("n_materials", static_cast<std::int64_t>(r.materials.size()));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
