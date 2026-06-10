"""Differential test for the tinyobjloader binding (Layer-1 + Layer-3).

The library's own attrib_t / shape_t / mesh_t / index_t / material_t are bound head-on;
the objtest::parse_obj fixture drives the out-param LoadObj over an in-memory .obj string
and returns an objtest::LoadResult aggregating those real types. oracle_native.cpp parses
the EXACT same .obj natively and emits every observable; the assertions compare the bound
module against that ground truth (vertex/normal/texcoord counts + values, shape names,
mesh face indices, num_face_vertices, material count). Layer 3 checks the bound surface
structurally (the real types' data members are present and readable).

NOTE: this run currently fails at Gate 4 (binding compile) -- the binder derives a bound
class's name from identifier_of(^^T), which is not a constant expression for tinyobj's
C-style `typedef struct {...} name;` types (the typedef sugar is lost at template-argument
substitution; ^^T resolves to the anonymous record). See findings_draft/. The test is
preserved as the intended differential.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("tinyobjloader_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# Same .obj as oracle_native.cpp (kObj), byte-for-byte.
OBJ = """
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
"""


def load():
    return m.parse_obj(OBJ, True)


def test_load_success_and_counts_differential():
    r = load()
    assert r.success == E["success"]
    assert len(r.attrib.vertices) == E["n_vertices_floats"]
    assert len(r.attrib.normals) == E["n_normals_floats"]
    assert len(r.attrib.texcoords) == E["n_texcoords_floats"]
    assert len(r.shapes) == E["n_shapes"]
    assert len(r.materials) == E["n_materials"]


def test_attrib_values_differential():
    r = load()
    assert r.attrib.vertices[0] == pytest.approx(E["v0_x"])
    assert r.attrib.vertices[6] == pytest.approx(E["v2_x"])
    assert r.attrib.normals[2] == pytest.approx(E["vn0_z"])


def test_shape_names_differential():
    r = load()
    assert r.shapes[0].name == E["shape0_name"]
    assert r.shapes[1].name == E["shape1_name"]


def test_mesh_indices_differential():
    r = load()
    mesh0 = r.shapes[0].mesh
    assert len(mesh0.indices) == E["shape0_n_indices"]
    assert len(mesh0.num_face_vertices) == E["shape0_n_faces"]
    assert mesh0.num_face_vertices[0] == E["shape0_face0_nverts"]
    assert mesh0.indices[0].vertex_index == E["shape0_idx0_v"]
    assert mesh0.indices[1].vertex_index == E["shape0_idx1_v"]
    assert mesh0.indices[2].vertex_index == E["shape0_idx2_v"]
    assert mesh0.indices[0].texcoord_index == E["shape0_idx0_vt"]
    assert mesh0.indices[0].normal_index == E["shape0_idx0_vn"]
    mesh1 = r.shapes[1].mesh
    assert mesh1.indices[2].vertex_index == E["shape1_idx2_v"]


# --- Layer 3: invariants (bound surface present on the real types) ---

def test_real_types_surface_bound():
    assert hasattr(m, "index_t") and hasattr(m, "mesh_t") and hasattr(m, "shape_t")
    assert hasattr(m, "attrib_t") and hasattr(m, "material_t")
    for f in ("vertices", "normals", "texcoords", "colors"):
        assert hasattr(m.attrib_t, f), f
    for f in ("indices", "num_face_vertices", "material_ids", "smoothing_group_ids"):
        assert hasattr(m.mesh_t, f), f
    for f in ("vertex_index", "normal_index", "texcoord_index"):
        assert hasattr(m.index_t, f), f
    for f in ("name", "mesh", "path"):
        assert hasattr(m.shape_t, f), f


def test_index_t_constructible():
    i = m.index_t()
    assert i.vertex_index is not None
