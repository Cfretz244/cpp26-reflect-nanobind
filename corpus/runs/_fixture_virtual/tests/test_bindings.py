"""Correctness test for the two-stage (codegen trampoline) shapes bindings.

Layer 1 (differential): the concrete Circle reproduces native-oracle area values.
Behavior (the point of codegen): a Python subclass overrides a C++ virtual and C++ dispatches
into it via the generated trampoline.
"""
import json
import math
import pathlib

import pytest

try:
    import shapes_ext as m
    HAVE = True
except ImportError:
    HAVE = False

needs = pytest.mark.skipif(not HAVE, reason="binding module not built")
EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


@needs
def test_differential_concrete_circle():
    c = m.Circle(2.0)
    assert c.area() == EXPECTED["circle2_area"]
    assert m.call_area(c) == EXPECTED["call_area_circle2"]
    assert c.name() == "circle"


@needs
def test_python_override_dispatched_from_cpp():
    """The trampoline must route a C++ virtual call into a Python override."""
    class Square(m.Shape):
        def __init__(self, side):
            super().__init__()
            self.side = side
        def area(self):
            return self.side * self.side
        def name(self):
            return "square"

    sq = Square(3.0)
    # called directly in Python
    assert sq.area() == 9.0
    # called from C++ through a Shape& reference -> must reach the Python override
    assert m.call_area(sq) == 9.0
    assert m.call_name(sq) == "square"


@needs
def test_non_pure_virtual_default_preserved():
    """Overriding only area() must keep the C++ default name() for a Python subclass."""
    class Blob(m.Shape):
        def area(self):
            return 1.0
    b = Blob()
    assert m.call_area(b) == 1.0
    assert m.call_name(b) == "shape"   # C++ default virtual still used
