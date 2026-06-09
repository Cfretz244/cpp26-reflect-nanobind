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
def test_invariants():
    """Layer 3: operator[] agrees with named components; members are read-write."""
    v = m.vecFloat3(7.0, 8.0, 9.0)
    assert (v[0], v[1], v[2]) == (v.x, v.y, v.z)   # __getitem__ matches components
    v.x = 100.0
    assert v.x == 100.0 and v[0] == 100.0          # write reflects in both views
    # default constructor zero-initializes
    z = m.vecFloat3()
    assert (z.x, z.y, z.z) == (0.0, 0.0, 0.0)
