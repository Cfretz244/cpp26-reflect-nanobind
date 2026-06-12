"""Differential + invariant correctness test for the linalg vec<float,N> bindings.

Layer 1 (differential): bindings must reproduce the native oracle's ground-truth values.
Layer 3 (invariants): operator[] must agree with named components; data members read-write.
"""
import json
import pathlib

import pytest

try:
    import linalg_ext as m
    HAVE = True
except ImportError:
    HAVE = False

needs = pytest.mark.skipif(not HAVE, reason="binding module not built")
EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


@needs
def test_differential_vec3():
    v = m.vecFloat3(1.5, 2.5, 3.5)
    assert v.x == EXPECTED["v_x"]
    assert v.y == EXPECTED["v_y"]
    assert v.z == EXPECTED["v_z"]
    assert v[0] == EXPECTED["v_idx0"]
    assert v[1] == EXPECTED["v_idx1"]
    assert v[2] == EXPECTED["v_idx2"]


@needs
def test_differential_vec2():
    w = m.vecFloat2(4.0, 5.0)
    assert w.x == EXPECTED["w_x"]
    assert w.y == EXPECTED["w_y"]
    assert w[0] == EXPECTED["w_idx0"]
    assert w[1] == EXPECTED["w_idx1"]


@needs
def test_differential_vec_grid():
    """The matcher-targeted instantiate_ grid: vec<{float,double,int},{2,3,4}>."""
    for t in ("Float", "Double", "Int"):
        for n in (2, 3, 4):
            assert hasattr(m, f"vec{t}{n}"), f"vec{t}{n}"
    vd = m.vecDouble3(0.5, 1.25, -2.0)
    assert (vd.x, vd.y, vd.z) == (EXPECTED["vd_x"], EXPECTED["vd_y"], EXPECTED["vd_z"])
    assert vd[2] == EXPECTED["vd_idx2"]
    vi = m.vecInt3(7, 8, 9)
    assert (vi.x, vi.y, vi.z) == (EXPECTED["vi_x"], EXPECTED["vi_y"], EXPECTED["vi_z"])
    vixy = vi.xy()                                  # truncation view -> vec<int,2>
    assert isinstance(vixy, m.vecInt2)
    assert (vixy.x, vixy.y) == (EXPECTED["vixy_x"], EXPECTED["vixy_y"])
    vs = m.vecInt4(5)                               # explicit scalar-fill ctor
    assert (vs.x, vs.w) == (EXPECTED["vs_x"], EXPECTED["vs_w"])


@needs
def test_differential_mat_grid():
    """The direct-target instantiate_ grid: mat<float,{2,3,4},{2,3,4}> (column-major)."""
    for r in (2, 3, 4):
        for c in (2, 3, 4):
            assert hasattr(m, f"matFloat{r}{c}"), f"matFloat{r}{c}"
    m22 = m.matFloat22(m.vecFloat2(1.0, 2.0), m.vecFloat2(3.0, 4.0))
    assert (m22[0].x, m22[0].y) == (EXPECTED["m22_c0x"], EXPECTED["m22_c0y"])
    assert (m22[1].x, m22[1].y) == (EXPECTED["m22_c1x"], EXPECTED["m22_c1y"])
    r0 = m22.row(0)
    assert isinstance(r0, m.vecFloat2)
    assert (r0.x, r0.y) == (EXPECTED["m22_r0x"], EXPECTED["m22_r0y"])
    # non-square pins the column-major semantics: mat<float,2,3> has three
    # vec2 columns and vec3 rows.
    m23 = m.matFloat23(m.vecFloat2(1.0, 2.0), m.vecFloat2(3.0, 4.0),
                       m.vecFloat2(5.0, 6.0))
    assert isinstance(m23[2], m.vecFloat2)
    assert (m23[2].x, m23[2].y) == (EXPECTED["m23_c2x"], EXPECTED["m23_c2y"])
    r1 = m23.row(1)
    assert isinstance(r1, m.vecFloat3)
    assert (r1.x, r1.y, r1.z) == (EXPECTED["m23_r1x"], EXPECTED["m23_r1y"],
                                  EXPECTED["m23_r1z"])
    mf = m.matFloat33(2.0)                          # explicit scalar-fill ctor
    assert (mf[1].y, mf[2].z) == (EXPECTED["mf_c1y"], EXPECTED["mf_c2z"])


@needs
def test_invariants():
    """Layer 3: operator[] agrees with named components; members are read-write."""
    v = m.vecFloat3(7.0, 8.0, 9.0)
    assert (v[0], v[1], v[2]) == (v.x, v.y, v.z)   # __getitem__ matches components
    v.x = 100.0
    assert v.x == 100.0 and v[0] == 100.0          # write reflects in both views
    # default constructor zero-initializes
    z = m.vecFloat3()
    assert (z.x, z.y, z.z) == (0.0, 0.0, 0.0)
    # identity_t (bound via the match_ marker that anchors the namespace
    # sweep): real API, constructible from its explicit int ctor.
    assert m.identity_t(1) is not None
    # mat columns are read-write data members
    mm = m.matFloat22(m.vecFloat2(0.0, 0.0), m.vecFloat2(0.0, 0.0))
    mm.x = m.vecFloat2(9.0, 8.0)
    assert (mm[0].x, mm[0].y) == (9.0, 8.0)
