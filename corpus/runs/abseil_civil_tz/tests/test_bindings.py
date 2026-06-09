"""Differential test for the Abseil civil-time/TimeZone binding (Layer-1 + Layer-3).

CivilDay/CivilSecond/CivilYear are REAL internal-namespace template specializations
(time_internal::cctz::detail::civil_time<tag>) bound under spec-derived Python names
(civil_timeDay_tag, ...). Field accessors, normalization, and member += (__iadd__)
bind directly; TimeZone + nested CivilInfo bind head-on; conversions go through the
tztest fixtures. oracle_native.cpp drives the identical surface natively. UTC/fixed
zones only (tzdata-independent).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_civil_tz_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# Spec-derived Python names for the civil aliases (CivilDay = civil_time<day_tag>, ...).
CivilDay = m.civil_timeDay_tag
CivilSecond = m.civil_timeSecond_tag
CivilYear = m.civil_timeYear_tag

# Ctor default-argument values are a C++26 gap: all six fields are required.
def day(y, mo=1, d=1):
    return CivilDay(y, mo, d, 0, 0, 0)


def test_civil_fields_differential():
    d = day(2015, 6, 13)
    assert d.year() == E["d_year"]
    assert d.month() == E["d_month"]
    assert d.day() == E["d_day"]
    assert m.weekday_index(d) == E["d_weekday"]    # Saturday


def test_civil_normalization_differential():
    n = day(2016, 2, 30)                            # normalizes to 2016-03-01
    assert n.year() == E["norm_year"]
    assert n.month() == E["norm_month"]
    assert n.day() == E["norm_day"]


def test_civil_iadd_differential():
    d = day(2015, 6, 13)
    d += 30                                         # member operator+= -> __iadd__
    assert d.year() == E["adv_year"]
    assert d.month() == E["adv_month"]
    assert d.day() == E["adv_day"]


def test_timezone_at_differential():
    z = m.utc()
    assert z.name() == E["utc_name"]
    ci = z.At(m.from_unix_seconds(0))
    assert ci.cs.year() == E["ci_year"]
    assert ci.cs.month() == E["ci_month"]
    assert ci.cs.day() == E["ci_day"]
    assert ci.cs.hour() == E["ci_hour"]
    assert ci.offset == E["ci_offset"]
    assert ci.is_dst == E["ci_dst"]
    assert ci.zone_abbr == E["ci_abbr"]             # const char* def_ro -> str


def test_fixed_zone_differential():
    plus1 = m.fixed(3600)
    ci = plus1.At(m.from_unix_seconds(0))
    assert ci.cs.hour() == E["ci1_hour"]
    assert ci.offset == E["ci1_offset"]


def test_civil_roundtrip_format_differential():
    cs = CivilSecond(2000, 1, 2, 3, 4, 5)
    assert m.to_unix_seconds(m.from_civil(cs, m.utc())) == E["rt_unix"]
    assert m.format_sec(m.from_unix_seconds(0), m.utc()) == E["fmt_epoch"]
    assert m.format_sec(m.from_unix_seconds(0), m.fixed(3600)) == E["fmt_epoch_plus1"]


def test_civil_year_truncation_differential():
    y = CivilYear(2015, 6, 13, 0, 0, 0)             # truncates below-year fields
    assert y.year() == E["y_year"]
    assert y.month() == E["y_month"]


# --- Layer 3: structural invariants ---

def test_load_timezone_optional():
    # LoadTimeZone -> optional<TimeZone> via the optional caster; named zones
    # depend on tzdata so only presence/absence is asserted.
    assert m.load("Not/AZone") is None
    nyc = m.load("America/New_York")
    if nyc is not None:
        assert nyc.name() == "America/New_York"


def test_to_civil_day():
    d = m.to_civil_day(m.from_unix_seconds(0), m.utc())
    assert (d.year(), d.month(), d.day()) == (1970, 1, 1)
