"""Differential + invariant correctness test for the POD fixture bindings.

Layer 1 (differential): assert the bindings reproduce the native oracle's ground-truth
values (tests/expected.json, produced by compiling+running oracle_native.cpp).
Layer 3 (invariants): properties that must hold regardless of exact values.
"""
import json
import math
import pathlib

import pytest

try:
    import pod_fixture_ext as m
    HAVE = True
except ImportError:
    HAVE = False

needs = pytest.mark.skipif(not HAVE, reason="binding module not built")

EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


@needs
def test_differential_against_native_oracle():
    """Layer 1: bindings must match values the native C++ harness produced."""
    assert m.Point(3.0, 4.0).norm2() == EXPECTED["norm2_3_4"]

    s = m.add(m.Point(1, 2), m.Point(3, 4))
    assert s.x == EXPECTED["add_x"]
    assert s.y == EXPECTED["add_y"]

    assert m.dot(m.Point(1, 2), m.Point(3, 4)) == EXPECTED["dot_12_34"]

    o = m.Point.origin()
    assert o.x == EXPECTED["origin_x"]
    assert o.y == EXPECTED["origin_y"]

    sc = m.Point(3.0, 4.0)
    sc.scale(2.0)
    assert sc.x == EXPECTED["scale2_x"]
    assert sc.y == EXPECTED["scale2_y"]

    assert int(m.Axis.X.value) == EXPECTED["axis_x"]
    assert int(m.Axis.Y.value) == EXPECTED["axis_y"]
    assert int(m.Axis.Z.value) == EXPECTED["axis_z"]


@needs
def test_data_members_readwrite():
    p = m.Point(1.0, 2.0)
    p.x = 10.0
    assert p.x == 10.0  # data member is read-write


@needs
def test_invariants():
    """Layer 3: metamorphic / algebraic properties, no oracle needed."""
    a, b = m.Point(1.5, -2.0), m.Point(0.25, 3.0)
    # add is commutative on each component
    assert m.add(a, b).x == m.add(b, a).x
    assert m.add(a, b).y == m.add(b, a).y
    # dot is symmetric
    assert m.dot(a, b) == m.dot(b, a)
    # norm2 == dot(p, p)
    assert math.isclose(a.norm2(), m.dot(a, a), rel_tol=1e-12)
    # scaling by k multiplies norm2 by k^2
    p = m.Point(3.0, 4.0)
    base = p.norm2()
    p.scale(2.0)
    assert math.isclose(p.norm2(), 4.0 * base, rel_tol=1e-12)
