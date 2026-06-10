"""Differential test for the HowardHinnant/date binding (Layer-1 + Layer-3).

date's calendrical value types are bound head-on. oracle_native.cpp drives the EXACT
same scenario through native date (same operator/ construction, comparisons, ok() checks,
weekday-from-date computation, sys_days serial round-trip, format()) and emits every
observable; the assertions compare the bound module against that ground truth value-by-
value. Layer 3 pins the Tier-2 themes: the value-type surface is present, the operator/
DSL yields the right composite types, and rich comparisons obey a total order.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("date_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


# --- Layer 1: value-type construction, accessors, operator<< (__str__) ---

def test_year_differential():
    y = m.year(2024)
    assert int(y) == E["year_int"]
    assert str(y) == E["year_str"]
    assert y.is_leap() == E["year_leap"]
    assert y.ok() == E["year_ok"]


def test_month_day_scalar_differential():
    assert int(m.month(2)) == E["month_int"]
    assert str(m.month(2)) == E["month_str"]
    assert int(m.day(29)) == E["day_int"]
    assert str(m.day(29)) == E["day_str"]


# --- Layer 1: operator/ calendar composition (the date DSL -> composite types) ---

def test_year_month_composition_differential():
    ym = m.year(2024) / m.month(2)
    assert str(ym) == E["ym_str"]
    assert int(ym.year()) == E["ym_year"]
    assert int(ym.month()) == E["ym_month"]


def test_month_day_composition_differential():
    md = m.month(7) / m.day(4)
    assert str(md) == E["md_str"]
    assert int(md.month()) == E["md_month"]
    assert int(md.day()) == E["md_day"]


def test_year_month_day_composition_differential():
    ymd = m.year(2024) / m.month(2) / m.day(29)
    assert str(ymd) == E["ymd_str"]
    assert int(ymd.year()) == E["ymd_year"]
    assert int(ymd.month()) == E["ymd_month"]
    assert int(ymd.day()) == E["ymd_day"]
    assert ymd.ok() == E["ymd_ok"]


# --- Layer 1: ok() validity ---

def test_ok_validity_differential():
    assert m.month(13).ok() == E["month13_ok"]
    assert m.day(32).ok() == E["day32_ok"]
    assert (m.year(2023) / m.month(2) / m.day(29)).ok() == E["feb29_2023_ok"]


# --- Layer 1: rich comparisons ---

def test_comparison_differential():
    assert (m.year(2024) < m.year(2020)) == E["y2024_lt_y2020"]
    assert (m.year(2024) > m.year(2020)) == E["y2024_gt_y2020"]
    assert (m.year(2024) == m.year(2024)) == E["y2024_eq_y2024"]
    assert (m.month(2) < m.month(3)) == E["feb_lt_mar"]
    assert ((m.year(2024)/m.month(1)/m.day(1)) < (m.year(2024)/m.month(2)/m.day(1))) == E["ymd_lt"]
    assert ((m.year(2024)/m.month(2)/m.day(29)) == m.serial_to_ymd(19782)) == E["ymd_eq"]


# --- Layer 1: sys_days serial round-trip (the fixture bridge) ---

def test_serial_roundtrip_differential():
    ymd = m.year(2024) / m.month(2) / m.day(29)
    serial = m.ymd_to_serial(ymd)
    assert serial == E["ymd_serial"]
    assert str(m.serial_to_ymd(serial)) == E["serial_back_str"]
    assert m.ymd_to_serial(m.year(1970)/m.month(1)/m.day(1)) == E["epoch_serial"]
    assert m.ymd_to_serial(m.year(2000)/m.month(1)/m.day(1)) == E["y2000_serial"]


# --- Layer 1: weekday computation from a date ---

def test_weekday_of_date_differential():
    ymd = m.year(2024) / m.month(2) / m.day(29)
    wd = m.weekday_of(ymd)
    assert str(wd) == E["wd_str"]
    assert wd.c_encoding() == E["wd_c_encoding"]
    assert wd.iso_encoding() == E["wd_iso_encoding"]
    wd_sun = m.weekday_of(m.year(2024) / m.month(3) / m.day(3))
    assert str(wd_sun) == E["wd_sun_str"]
    assert wd_sun.c_encoding() == E["wd_sun_c"]
    assert wd_sun.iso_encoding() == E["wd_sun_iso"]


# --- Layer 1: formatted output (date::format via the fixture) ---

def test_format_differential():
    ymd = m.year(2024) / m.month(2) / m.day(29)
    wd = m.weekday_of(ymd)
    assert m.format_ymd("%Y-%m-%d", ymd) == E["fmt_iso"]
    assert m.format_ymd("%B %d, %Y", ymd) == E["fmt_long"]
    assert m.format_weekday("%A", wd) == E["fmt_wd_full"]
    assert m.format_weekday("%a", wd) == E["fmt_wd_abbr"]


# --- Layer 3: invariants (the Tier-2 themes, structurally) ---

def test_value_surface_bound():
    for meth in ("ok",):
        for cls in (m.year, m.month, m.day, m.year_month, m.month_day, m.year_month_day):
            assert hasattr(cls, meth), (cls, meth)
    assert hasattr(m.year, "is_leap")
    assert hasattr(m.weekday, "c_encoding")
    assert hasattr(m.weekday, "iso_encoding")
    # the value types carry int conversions and rich comparison
    for dunder in ("__int__", "__eq__", "__lt__", "__str__"):
        assert hasattr(m.day, dunder), dunder


def test_operator_slash_yields_composites():
    # the calendar DSL: year/month -> year_month, month/day -> month_day,
    # year/month/day -> year_month_day (real bound composite types, not tuples).
    assert isinstance(m.year(2024) / m.month(2), m.year_month)
    assert isinstance(m.month(2) / m.day(1), m.month_day)
    assert isinstance(m.year(2024) / m.month(2) / m.day(1), m.year_month_day)


def test_total_order_invariant():
    # rich comparisons form a strict total order over a sorted day sequence.
    days = [m.day(i) for i in (3, 1, 2, 5, 4)]
    s = sorted(days, key=lambda x: int(x))
    for a, b in zip(s, s[1:]):
        assert a < b
        assert not (b < a)
        assert a != b


def test_equality_consistency_invariant():
    a = m.year(2024) / m.month(2) / m.day(29)
    b = m.serial_to_ymd(m.ymd_to_serial(a))
    assert a == b
    assert not (a < b) and not (b < a)
