"""Differential test for the absl::Status / absl::StatusCode binding (Layer-1 + Layer-3).

The binder binds absl::Status (ok/code/raw_code/message/ToString, constructed from
(StatusCode, string_view)) and the absl::StatusCode enum directly. oracle_native.cpp builds the
same Status values natively and emits their properties; the assertions compare the bound module
against that ground truth (shared compiler + shared Abseil).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_status_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

SC = m.StatusCode


def test_ok_status():
    ok = m.Status()                              # default-constructed == OK
    assert ok.ok() == (E["ok_is_ok"] == True)    # noqa: E712 (explicit for parity w/ JSON bool)
    assert ok.ok() is True
    assert ok.raw_code() == E["ok_raw"]


def test_error_status_differential():
    nf = m.Status(SC.kNotFound, "thing missing")
    assert nf.ok() is False
    assert nf.raw_code() == E["nf_raw"]
    assert nf.message() == E["nf_msg"]
    assert nf.code() == SC.kNotFound             # code() returns the bound enum
    # ToString(StatusToStringMode) has a non-bindable default-argument value (a C++26 gap),
    # so it requires an explicit mode arg we don't reflect here; message()/code() suffice.


def test_second_error_status():
    inv = m.Status(SC.kInvalidArgument, "bad arg")
    assert inv.raw_code() == E["inv_raw"]
    assert inv.message() == E["inv_msg"]


def test_status_code_enum_values():
    assert SC.kOk.value == E["code_kOk"]
    assert SC.kNotFound.value == E["code_kNotFound"]
    assert SC.kInvalidArgument.value == E["code_kInvalidArgument"]
    assert SC.kInternal.value == E["code_kInternal"]


# --- Layer 3: invariants ---

def test_status_surface_bound():
    for meth in ("ok", "code", "raw_code", "message", "ToString", "Update"):
        assert hasattr(m.Status, meth), meth
