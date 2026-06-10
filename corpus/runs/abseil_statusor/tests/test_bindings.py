"""Differential test for the Abseil StatusOr<T> binding (Layer-1 differential + Layer-3).

Three real specializations (StatusOr<int>/StatusOr<std::string>/StatusOr<double>) bind
directly: ok(), status() -> bound absl::Status, IgnoreError(), and -- since the binder
gained entity-proxy support (BINDER-0009, -fentity-proxy-reflection) -- value() itself,
bound through its using-redeclaration from the PRIVATE internal_statusor::OperatorBase
base. Construction still goes through the sotest factories (StatusOr's converting ctors
are templates, skipped by design); the get_* fixtures remain as a cross-check channel.
oracle_native.cpp drives the identical surface.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_statusor_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def test_ok_int_differential():
    oi = m.ok_int(42)
    assert oi.ok() == E["oi_ok"]
    assert oi.value() == E["oi_value"]          # bound via entity proxy
    assert m.get_int(oi) == E["oi_value"]       # fixture channel agrees
    assert oi.status().raw_code() == E["oi_code"]       # kOk == 0
    assert oi.status().code() == m.StatusCode.kOk


def test_err_int_differential():
    ei = m.err_int(m.StatusCode.kNotFound, "no int here")
    assert ei.ok() == E["ei_ok"]
    st = ei.status()
    assert st.raw_code() == E["ei_raw"]
    assert st.message() == E["ei_msg"]


def test_string_specialization_differential():
    os_ = m.ok_str("hello statusor")
    assert os_.ok() == E["os_ok"]
    assert os_.value() == E["os_value"]
    es = m.err_str(m.StatusCode.kInvalidArgument, "bad string")
    assert es.ok() == E["es_ok"]
    assert es.status().raw_code() == E["es_raw"]
    assert es.status().message() == E["es_msg"]


def test_double_specialization_differential():
    od = m.ok_dbl(2.5)
    assert od.ok() == E["od_ok"]
    assert od.value() == E["od_value"]
    ed = m.err_dbl(m.StatusCode.kInternal, "bad double")
    assert ed.ok() == E["ed_ok"]
    assert ed.status().raw_code() == E["ed_raw"]


def test_default_ctor_is_unknown_error():
    # explicit StatusOr() is documented to hold kUnknown; the default ctor binds.
    d = m.StatusOrInt()
    assert d.ok() == E["def_ok"]
    assert d.status().raw_code() == E["def_raw"]


# --- Layer 3: structural invariants ---

def test_value_on_error_raises():
    # value() on an error StatusOr throws absl::BadStatusOrAccess -> Python exception,
    # both through the directly bound proxy and the fixture channel.
    with pytest.raises(Exception):
        m.err_int(m.StatusCode.kNotFound, "nope").value()
    with pytest.raises(Exception):
        m.get_int(m.err_int(m.StatusCode.kNotFound, "nope"))


def test_spec_python_names():
    # CamelCase spec naming: StatusOr<int> -> StatusOrInt, <std::string> ->
    # StatusOrString, <double> -> StatusOrDouble.
    for name in ("StatusOrInt", "StatusOrString", "StatusOrDouble"):
        assert isinstance(getattr(m, name), type), name
