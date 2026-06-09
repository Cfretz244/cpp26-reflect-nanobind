"""Differential test for the Abseil Duration/Time binding (Layer-1 + Layer-3).

absl::Duration / absl::Time are bound directly: their arithmetic/comparison operators map to
Python dunders, operator<< -> __str__. The timetest fixtures supply the factory/conversion free
functions (Duration/Time have no public integer constructors). oracle_native.cpp runs the same
operations natively; assertions compare the bound module against that ground truth.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_time_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def test_duration_construct_and_convert():
    assert m.to_seconds(m.seconds(90)) == E["s90_to_seconds"]
    assert m.to_minutes(m.seconds(120)) == E["s120_to_minutes"]
    assert m.to_seconds(m.milliseconds(1500)) == E["ms_1500_to_s"]


def test_duration_operators_differential():
    # Real Duration operators bound on the type (free operator+/-, comparison).
    assert m.to_seconds(m.seconds(10) + m.seconds(5)) == E["sum_10_5"]
    assert m.to_seconds(m.seconds(100) - m.seconds(40)) == E["diff_100_40"]
    assert (m.minutes(2) == m.seconds(120)) == E["min2_eq_s120"]
    assert (m.seconds(90) < m.hours(1)) == E["s90_lt_h1"]


def test_duration_str():
    # operator<<(ostream&, Duration) surfaced as __str__.
    assert str(m.seconds(90)) == E["s90_str"]      # "1m30s"


def test_time_roundtrip_differential():
    assert m.to_unix_seconds(m.from_unix_seconds(1000000000)) == E["unix_roundtrip"]
    assert m.to_unix_seconds(m.from_unix_seconds(1000) + m.seconds(5)) == E["time_plus_dur"]


# --- Layer 3: invariants ---

def test_duration_dunders_bound():
    for d in ("__add__", "__sub__", "__lt__", "__eq__", "__str__"):
        assert hasattr(m.Duration, d), d
