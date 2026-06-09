"""Differential + invariant test for a recursive data structure (Node with vector<Node>).

This run is COVERAGE for self-referential types, not a regression test for BINDER-0004:
the same type builds on the pre-BINDER-0004 binder too (verified), i.e. self-reference never
caused infinite recursion. It does exercise binding a std::vector<Self> member and recursion.
"""
import json
import pathlib
import pytest

try:
    import tree_ext as m
    HAVE = True
except ImportError:
    HAVE = False

needs = pytest.mark.skipif(not HAVE, reason="binding module not built")
EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


def _tree():
    root = m.Node(10)
    a, b = m.Node(3), m.Node(5)
    a.add(m.Node(1))
    root.add(a)
    root.add(b)
    return root, a


@needs
def test_differential():
    root, a = _tree()
    assert root.total() == EXPECTED["root_total"]     # 19
    assert root.count() == EXPECTED["root_count"]     # 2
    assert a.total() == EXPECTED["a_total"]           # 4
    assert root.children[0].value == EXPECTED["child0_value"]   # 3


@needs
def test_invariants():
    root, _ = _tree()
    # the self-referential member is bound as a list of Node
    assert len(root.children) == 2
    assert isinstance(root.children[0], m.Node)
    # total() == value + sum(child.total())
    assert root.total() == root.value + sum(c.total() for c in root.children)
