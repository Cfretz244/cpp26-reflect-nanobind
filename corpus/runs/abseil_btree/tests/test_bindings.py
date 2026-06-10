"""Differential test for the Abseil btree binding (Layer-1 + Layer-3).

btree_map/btree_set bound head-on (the ordered half of BINDER-0008): the
heterogeneous query surface (contains/count/at/erase/operator[]) binds via
default-instantiable member templates; population goes through bttest::put/add,
and the ORDER is observed through the bttest first_key/first_elem fixtures
(iteration itself is C++-only until an __iter__ feature exists).
oracle_native.cpp drives the identical surface natively.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_btree_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

BM = next(getattr(m, n) for n in dir(m) if n.startswith("btree_map"))
BS = next(getattr(m, n) for n in dir(m) if n.startswith("btree_set"))


def test_btree_map_differential():
    d = BM()
    m.put(d, 30, "thirty")
    m.put(d, 10, "ten")
    m.put(d, 20, "twenty")
    assert d.size() == E["m_size"]
    assert d.contains(10) == E["m_has10"]
    assert d.contains(99) == E["m_has99"]
    assert d.at(10) == E["m_at10"]
    assert d[20] == E["m_idx20"]
    assert m.first_key(d) == E["m_first_key"]          # sorted despite insert order
    assert d[5] == E["m_idx5_default"]                 # default-inserts
    assert m.first_key(d) == E["m_first_key_after5"]   # new minimum observed
    assert d.erase(10) == E["m_erase10"]
    assert d.size() == E["m_size_after"]
    assert d.count(20) == E["m_count20"]
    d.clear()
    assert d.empty() == E["m_cleared_empty"]


def test_btree_set_differential():
    s = BS()
    m.add(s, 42)
    m.add(s, 7)
    m.add(s, 42)
    m.add(s, 19)
    assert s.size() == E["s_size"]
    assert m.first_elem(s) == E["s_first"]             # ordered: minimum first
    assert s.contains(42) == E["s_has42"]
    assert s.contains(8) == E["s_has8"]
    assert s.erase(7) == E["s_erase7"]
    assert m.first_elem(s) == E["s_first_after"]
    assert s.size() == E["s_size_after"]


# --- Layer 3: structural invariants ---

def test_query_api_surface():
    for meth in ("contains", "count", "find", "erase", "at", "equal_range",
                 "lower_bound", "upper_bound", "__getitem__"):
        assert hasattr(BM, meth), meth


def test_at_missing_key_raises():
    with pytest.raises(Exception):
        BM().at(42)
