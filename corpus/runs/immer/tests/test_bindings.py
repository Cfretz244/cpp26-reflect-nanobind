"""Differential test for the immer binding (Layer-1 + Layer-3).

immer's persistent (immutable) containers are bound head-on: vector<int>, vector<string>,
map<int,int>, set<int>. Their defining property is value semantics with structural sharing
-- push_back/set/insert/erase RETURN A NEW container, leaving the receiver untouched.

oracle_native.cpp drives the EXACT same chain of persistent updates through native immer and
emits the contents of EVERY intermediate version plus the size/count/find/at surfaces; the
Layer-1 assertions compare the bound module against that ground truth value-by-value. The
core invariant -- the ORIGINAL is unchanged after each mutating call -- is asserted both
inside the differential (originals appear in expected.json) and structurally in Layer 3.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("immer_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def _cls(prefix):
    names = [n for n in dir(m) if n.startswith(prefix)]
    assert len(names) == 1, (prefix, names)
    return getattr(m, names[0])


IVec = _cls("vectorIntMemory")
SVec = _cls("vectorStringMemory")
IMap = _cls("mapInt")
ISet = _cls("setInt")


def contents(v):
    """Read a bound immer::vector out to a Python list via size()/__getitem__."""
    return [v[i] for i in range(v.size())]


# ----------------------- vector<int> persistent chain -----------------------

def test_vector_persistent_chain_differential():
    v0 = IVec()
    v1 = v0.push_back(10)
    v2 = v1.push_back(20)
    v3 = v2.push_back(30)
    v3b = v3.set(1, 99)
    v3t = v3.take(2)

    # every intermediate version's contents match the native oracle
    # (the oracle emits these as JSON arrays, so E[...] is already a list)
    assert contents(v0) == E["v0"]
    assert contents(v1) == E["v1"]
    assert contents(v2) == E["v2"]
    assert contents(v3) == E["v3"]
    assert contents(v3b) == E["v3b"]
    assert contents(v3t) == E["v3t"]

    # structural-sharing-as-value-semantics: originals untouched after set/take
    assert contents(v3) == [10, 20, 30]
    assert contents(v2) == [10, 20]
    assert v0.size() == 0

    assert v3.size() == E["v3_size"]
    assert v0.empty() == E["v0_empty"]
    assert v3.empty() == E["v3_empty"]
    assert v3.front() == E["v3_front"]
    assert v3.back() == E["v3_back"]
    assert v3.at(1) == E["v3_at1"]


def test_vector_value_equality_differential():
    v3 = IVec().push_back(10).push_back(20).push_back(30)
    v2 = IVec().push_back(10).push_back(20)
    assert (v2 == v3.take(2)) == E["v2_eq_take2_of_v3"]
    assert (v2 == v3) == E["v2_eq_v3"]
    # eq is genuine value equality across independently-built versions
    assert v2 == v3.take(2)
    assert v2 != v3


def test_vector_at_out_of_range_raises():
    v = IVec().push_back(5)
    with pytest.raises(IndexError):
        v.at(9)


def test_vector_fill_ctor_uses_parens_not_initializer_list():
    """BINDER-0026: reflected ctors construct with PARENS (reflect_init), not braces.

    immer::vector<int> carries BOTH a fill ctor `vector(size_type n, T v={})` and an
    `initializer_list<int>` ctor. nb::init's braced `Type{n, v}` would hijack toward the
    initializer_list ctor and narrow `size_type -> int` (a hard error); reflect_init's
    parenthesized `Type(n, v)` selects exactly the fill ctor. The bound module exposes the
    fill ctor and it must produce n copies of v -- NOT a single-element list of {n}.
    """
    v = IVec(3, 7)
    assert contents(v) == [7, 7, 7]      # fill ctor: 3 copies of 7, not [3]
    assert v.size() == 3


def test_vector_string_persistent_differential():
    s2 = SVec().push_back("alpha").push_back("beta")
    s2b = s2.set(0, "ALPHA")
    assert contents(s2) == E["s2"]
    assert contents(s2b) == E["s2b"]
    assert s2.front() == E["s2_front"]
    assert s2.back() == E["s2_back"]
    # original untouched after set
    assert contents(s2) == ["alpha", "beta"]


# ----------------------- map<int,int> persistent chain -----------------------

def test_map_persistent_chain_differential():
    m0 = IMap()
    m1 = m0.set(1, 100)
    m2 = m1.set(2, 200)
    m3 = m2.set(1, 111)
    m3e = m3.erase(2)

    assert m0.size() == E["m0_size"]
    assert m2.size() == E["m2_size"]
    assert m3.size() == E["m3_size"]
    assert m3e.size() == E["m3e_size"]

    # original m2 keeps key 1 == 100 after m3 overwrote it to 111
    assert m2[1] == E["m2_at1"] == 100
    assert m3[1] == E["m3_at1"] == 111
    assert m2.count(2) == E["m2_count2"]
    assert m3e.count(2) == E["m3e_count2"]
    # absent key -> default-constructed value (0)
    assert m2[99] == E["m2_get_missing"] == 0


def test_map_find_differential():
    m2 = IMap().set(1, 100).set(2, 200)
    assert m2.find(2) == E["m2_find2"] == 200
    assert (m2.find(99) is None) == E["m2_find99_null"]


def test_map_insert_pair_differential():
    m3e = IMap().set(1, 111)
    m4 = m3e.insert((5, 500))
    assert m4[5] == E["m4_at5"] == 500
    assert m3e.count(5) == E["m3e_count5"] == 0   # original lacks 5


def test_map_at_missing_raises():
    mp = IMap().set(1, 11)
    with pytest.raises((IndexError, KeyError)):
        mp.at(99)


# ----------------------- set<int> persistent chain -----------------------

def test_set_persistent_chain_differential():
    t0 = ISet()
    t1 = t0.insert(7)
    t2 = t1.insert(8)
    t3 = t2.insert(7)           # duplicate
    t3e = t3.erase(8)

    assert t0.size() == E["t0_size"]
    assert t2.size() == E["t2_size"]
    assert t3.size() == E["t3_size"] == 2       # duplicate insert did not grow
    assert t3e.size() == E["t3e_size"]
    assert t2.count(7) == E["t2_count7"]
    assert t2.count(9) == E["t2_count9"]
    assert (t2.find(8) is not None) == E["t2_find8_present"]
    assert (t2.find(9) is None) == E["t2_find9_null"]
    assert t3e.count(8) == E["t3e_count8"]
    # original t2 still contains 8 after t3e erased it
    assert t2.count(8) == E["t2_count8_after_erase"] == 1


# ----------------------- Layer 3: structural invariants -----------------------

def test_surface_present():
    for meth in ("size", "empty", "push_back", "set", "take", "at", "back", "front"):
        assert hasattr(IVec, meth), meth
    for meth in ("size", "count", "find", "set", "insert", "erase", "at"):
        assert hasattr(IMap, meth), meth
    for meth in ("size", "count", "find", "insert", "erase"):
        assert hasattr(ISet, meth), meth


def test_originals_immutable_under_mutation():
    """The persistence invariant, isolated: every mutating method returns a NEW object
    and never mutates the receiver."""
    v = IVec().push_back(1).push_back(2)
    before = contents(v)
    _ = v.push_back(3)
    _ = v.set(0, 99)
    _ = v.take(1)
    assert contents(v) == before == [1, 2]

    mp = IMap().set(1, 10)
    _ = mp.set(1, 20)
    _ = mp.erase(1)
    assert mp[1] == 10 and mp.count(1) == 1

    s = ISet().insert(1).insert(2)
    _ = s.insert(3)
    _ = s.erase(1)
    assert s.count(1) == 1 and s.count(3) == 0 and s.size() == 2


def test_returned_versions_are_independent_objects():
    v0 = IVec()
    v1 = v0.push_back(1)
    assert v0 is not v1
    assert isinstance(v1, IVec)
    assert v0.size() == 0 and v1.size() == 1
