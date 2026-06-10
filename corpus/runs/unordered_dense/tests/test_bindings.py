"""Differential test for the ankerl::unordered_dense binding (Layer-1 + Layer-3).

map<int,std::string> and set<int> bound head-on (the library's public alias
templates, which resolve to detail::table specializations). The query surface
(contains/count/at/erase/operator[]) binds off the spec; population goes through
udtest::put/add (insert()'s pair<iterator,bool> return has no Python
representation). oracle_native.cpp drives the identical surface natively.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("unordered_dense_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# The public alias templates map<int,std::string>/set<int> resolve to the
# underlying detail::table<...> specializations, so the generated Python names
# are the table-spec mangles ("tableIntString..." for the map, "tableIntVoid..."
# for the set, T=void). Resolve them by the distinguishing key/value substrings
# rather than a load-bearing exact name.
_tables = [n for n in dir(m) if n.startswith("table")]
MAP = getattr(m, next(n for n in _tables if "IntString" in n))
SET = getattr(m, next(n for n in _tables if "IntVoid" in n and "String" not in n))


def test_map_differential():
    d = MAP()
    m.put(d, 1, "one")
    m.put(d, 2, "two")
    assert d.size() == E["m_size"]
    assert d.contains(1) == E["m_has1"]
    assert d.contains(3) == E["m_has3"]
    assert d.at(1) == E["m_at1"]
    assert d[2] == E["m_idx2"]                      # __getitem__
    assert d.count(3) == E["m_count3"]
    assert d[9] == E["m_idx9_default"]              # C++ semantics: default-inserts
    assert d.size() == E["m_size_after_idx9"]
    assert d.erase(1) == E["m_erase1"]
    assert d.erase(1) == E["m_erase1_again"]
    assert d.size() == E["m_size_after_erase"]
    m.put(d, 2, "TWO")
    assert d.at(2) == E["m_at2_overwritten"]
    assert d.empty() == E["m_empty_before_clear"]
    d.clear()
    assert d.empty() == E["m_cleared_empty"]


def test_map_growth_differential():
    g = MAP()
    for i in range(100):
        m.put(g, i, "v")
    assert g.size() == E["m_grown_size"]
    assert g.bucket_count() == E["m_grown_bucket_count"]  # same header => same growth


def test_set_differential():
    s = SET()
    m.add(s, 7)
    m.add(s, 7)                                    # duplicate: no-op
    m.add(s, 8)
    assert s.size() == E["s_size"]
    assert s.contains(7) == E["s_has7"]
    assert s.contains(9) == E["s_has9"]
    assert s.count(7) == E["s_count7"]
    assert s.erase(7) == E["s_erase7"]
    assert s.erase(7) == E["s_erase7_again"]
    assert s.size() == E["s_size_after"]
    s.clear()
    assert s.empty() == E["s_empty_after_clear"]


# --- Layer 3: structural invariants ---

def test_query_api_surface():
    for meth in ("contains", "count", "at", "erase", "size", "empty", "clear",
                 "__getitem__", "bucket_count"):
        assert hasattr(MAP, meth), meth


def test_at_missing_key_raises():
    with pytest.raises(Exception):
        MAP().at(42)                               # std::out_of_range -> Python


def test_no_detail_internals_leaked():
    # Reachability: the bucket / segmented internals must not appear as bound
    # module attributes in their own right.
    for n in dir(m):
        assert not n.lower().startswith("bucket"), n
        assert not n.lower().startswith("segmented"), n
