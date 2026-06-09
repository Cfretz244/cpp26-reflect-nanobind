"""Differential + invariant correctness test for the glm vec subset.

glm stores components in an anonymous union, so x/y/z/w are NOT exposed as named attributes
(see finding BINDER-0002); the usable surface is constructors, operator[] -> __getitem__, and
the static length(). Classes are looked up by prefix because the auto-generated CamelCase name
is cosmetically broken for glm's enum qualifier arg (finding BINDER-0003).
"""
import json
import pathlib

import pytest

try:
    import glm_ext as m
    HAVE = True
except ImportError:
    HAVE = False

needs = pytest.mark.skipif(not HAVE, reason="binding module not built")
EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


def _cls(prefix):
    names = [n for n in dir(m) if n.startswith(prefix)]
    assert names, f"no bound class starting with {prefix!r}"
    return getattr(m, names[0])


@needs
def test_differential_vec3():
    V3 = _cls("vec3")
    a = V3(1.5, 2.5, 3.5)               # component constructor
    assert (a[0], a[1], a[2]) == (EXPECTED["a0"], EXPECTED["a1"], EXPECTED["a2"])
    b = V3(4.0)                         # scalar-broadcast constructor
    assert (b[0], b[1], b[2]) == (EXPECTED["b0"], EXPECTED["b1"], EXPECTED["b2"])
    assert V3.length() == EXPECTED["len3"]


@needs
def test_differential_vec4():
    V4 = _cls("vec4")
    c = V4(1.0, 2.0, 3.0, 4.0)
    assert (c[0], c[1], c[2], c[3]) == (
        EXPECTED["c0"], EXPECTED["c1"], EXPECTED["c2"], EXPECTED["c3"])
    assert V4.length() == EXPECTED["len4"]


@needs
def test_invariants():
    V3 = _cls("vec3")
    # scalar broadcast: all components equal
    b = V3(7.0)
    assert b[0] == b[1] == b[2] == 7.0
    # length() is the component count, independent of values
    assert V3.length() == 3
    # default construct is indexable
    z = V3()
    _ = (z[0], z[1], z[2])
