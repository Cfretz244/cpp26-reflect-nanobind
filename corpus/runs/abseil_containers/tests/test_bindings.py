"""Differential test for the Abseil InlinedVector binding (Layer-1 differential + Layer-3 invariants).

The binder reflects two specializations of absl::InlinedVector directly — InlinedVectorInt4
(<int,4>) and InlinedVectorDouble2 (<double,2>) — fully-specialized concrete data structures from
the real, non-header-only Abseil library (no fixture wrappers). The absl runtime symbol
ThrowStdOutOfRange (referenced by at()) is linked in from throw_delegate.cc via meta.extra_sources.

Layer 1 (differential): oracle_native.cpp drives the SAME operations through absl::InlinedVector
natively and emits ground truth as expected.json; every value assertion compares the bound module
against it (including the small-buffer-optimization capacity growth, which matches because both use
one Abseil build). Layer 3 adds structural invariants (empty/clear, bounds-checked at() raising).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_ext")

_EXPECTED_PATH = pathlib.Path(__file__).parent / "expected.json"
if not _EXPECTED_PATH.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)",
                allow_module_level=True)
E = _json.loads(_EXPECTED_PATH.read_text())


def test_int_vector_differential():
    v = m.InlinedVectorInt4()
    for x in (10, 20, 30):
        v.push_back(x)
    assert v.size() == E["iv_size"]
    assert [v[0], v[1], v[2]] == [E["iv0"], E["iv1"], E["iv2"]]   # operator[] -> __getitem__
    assert v.at(1) == E["iv1"]                                    # bounds-checked accessor
    assert v.front() == E["iv_front"]
    assert v.back() == E["iv_back"]
    assert v.capacity() == E["iv_cap"]                           # inline buffer == 4
    v.push_back(40)
    v.push_back(50)                                              # spill past the inline buffer
    assert v.size() == E["iv_size5"]
    assert v.capacity() == E["iv_cap5"]                         # heap-grown capacity (matches native)
    v.pop_back()
    assert v.size() == E["iv_pop_size"]
    assert v.back() == E["iv_pop_back"]


def test_double_vector_specialization():
    d = m.InlinedVectorDouble2()
    d.push_back(1.5)
    d.push_back(2.5)
    assert d.size() == E["dv_size"]
    assert d[0] == E["dv0"]
    assert d[1] == E["dv1"]
    assert d.capacity() == E["dv_cap"]                          # inline buffer == 2


# --- Layer 3: structural invariants (independent of the oracle) ---

def test_inlined_vector_invariants():
    v = m.InlinedVectorInt4()
    assert v.empty()
    v.push_back(7)
    assert not v.empty()
    assert v.size() == 1 and v[0] == 7
    v.clear()
    assert v.empty() and v.size() == 0


def test_at_bounds_check_raises():
    # at() out of range routes through absl::base_internal::ThrowStdOutOfRange (linked from
    # throw_delegate.cc); nanobind translates std::out_of_range to IndexError.
    v = m.InlinedVectorInt4()
    v.push_back(1)
    with pytest.raises(IndexError):
        v.at(5)


def test_two_specializations_bound():
    # Distinct template specializations bind to distinct, CamelCase-named Python types.
    assert hasattr(m, "InlinedVectorInt4")
    assert hasattr(m, "InlinedVectorDouble2")
    assert m.InlinedVectorInt4 is not m.InlinedVectorDouble2
