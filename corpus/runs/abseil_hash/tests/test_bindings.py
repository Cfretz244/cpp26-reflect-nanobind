"""Differential test for the Abseil hash-container binding (Layer-1 + Layer-3).

The BINDER-0008 payoff: flat_hash_map/flat_hash_set/node_hash_map bound head-on.
The heterogeneous query surface (contains/count/at/erase/operator[]) binds via
default-instantiable member templates; population goes through hmtest::put/add
(insert()'s pair<iterator,bool> return has no Python representation).
oracle_native.cpp drives the identical surface natively.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_hash_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# Spec-derived Python names; resolved by prefix so the (cosmetic) default-arg
# suffix in the name is not load-bearing for the tests.
FHM = next(getattr(m, n) for n in dir(m) if n.startswith("flat_hash_map"))
FHS = next(getattr(m, n) for n in dir(m) if n.startswith("flat_hash_set"))
NHM = next(getattr(m, n) for n in dir(m) if n.startswith("node_hash_map"))


def test_flat_hash_map_differential():
    d = FHM()
    m.put(d, 1, "one")
    m.put(d, 2, "two")
    assert d.size() == E["m_size"]
    assert d.contains(1) == E["m_has1"]
    assert d.contains(3) == E["m_has3"]
    assert d.at(1) == E["m_at1"]
    assert d[2] == E["m_idx2"]                      # __getitem__ (member template)
    assert d.count(3) == E["m_count3"]
    assert d[9] == E["m_idx9_default"]              # C++ semantics: default-inserts
    assert d.size() == E["m_size_after_idx9"]
    assert d.erase(1) == E["m_erase1"]
    assert d.erase(1) == E["m_erase1_again"]
    assert d.size() == E["m_size_after_erase"]
    m.put(d, 2, "TWO")
    assert d.at(2) == E["m_at2_overwritten"]
    d.clear()
    assert d.empty() == E["m_cleared_empty"]


def test_flat_hash_map_growth_differential():
    g = FHM()
    for i in range(100):
        m.put(g, i, "v")
    assert g.size() == E["m_grown_size"]
    assert g.capacity() == E["m_grown_capacity"]    # same abseil build => same growth
    assert g.bucket_count() == E["m_bucket_count"]


def test_flat_hash_set_differential():
    s = FHS()
    m.add(s, 7)
    m.add(s, 7)                                     # duplicate: no-op
    m.add(s, 8)
    assert s.size() == E["s_size"]
    assert s.contains(7) == E["s_has7"]
    assert s.contains(9) == E["s_has9"]
    assert s.erase(7) == E["s_erase7"]
    assert s.size() == E["s_size_after"]


def test_node_hash_map_differential():
    n = NHM()
    m.put_node(n, 5, "five")
    m.put_node(n, 6, "six")
    assert n.size() == E["n_size"]
    assert n.contains(5) == E["n_has5"]
    assert n.at(5) == E["n_at5"]
    assert n.erase(5) == E["n_erase5"]
    assert n[6] == E["n_idx6"]


# --- Layer 3: structural invariants ---

def test_no_policy_explosion():
    # Reachability discovery: the container_internal POLICY types must not be
    # registered (FlatHashMapPolicy and friends appear only inside other specs'
    # Python names, never as module attributes themselves).
    for n in dir(m):
        assert not n.startswith("FlatHashMapPolicy"), n
        assert not n.startswith("FlatHashSetPolicy"), n
        assert not n.startswith("NodeHashMapPolicy"), n
        assert not n.startswith("Hash_policy_traits"), n
        assert not n.startswith("Map_slot_policy"), n


def test_query_api_surface():
    # The heterogeneous (member-template) query surface is present on the map.
    for meth in ("contains", "count", "find", "erase", "at", "equal_range",
                 "__getitem__"):
        assert hasattr(FHM, meth), meth


def test_at_missing_key_raises():
    with pytest.raises(Exception):
        FHM().at(42)                                # std::out_of_range -> Python
