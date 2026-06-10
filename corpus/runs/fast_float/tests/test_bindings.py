"""Differential test for the fast_float binding (Layer-1 + Layer-3).

fast_float's parse front is free function templates over (first, last, &out) -- not
Python-expressible -- so the fixture namespace wraps each concrete instantiation
(double/float/int64/uint64) as string-in -> {value, consumed, ok, ec}-out. The library's
own value types (chars_format enum, parse_options_t<char>, from_chars_result_t<char>) are
reflected head-on. oracle_native.cpp drives the EXACT same input strings / formats / options
through native fast_float and emits every observable; the assertions below compare the bound
module against that ground truth (parsed values incl. subnormals/ties/overflow/inf-nan,
chars consumed, ok flags, errc codes, format-gating, custom decimal point), plus Layer-3
invariants on the head-on-bound types.
"""
import json as _json
import math
import pathlib

import pytest

m = pytest.importorskip("fast_float_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

CF = m.chars_format
G, H, S, F = CF.general, CF.hex, CF.scientific, CF.fixed


def _num(x):
    """Decode the oracle's value repr (handles the "inf"/"nan"/"-inf" string sentinels)."""
    if isinstance(x, str):
        return {"inf": math.inf, "-inf": -math.inf, "nan": math.nan}[x]
    return float(x)


def _assert_double(key, r):
    exp_v = _num(E[key + "_value"])
    if math.isnan(exp_v):
        assert math.isnan(r.value), key
    else:
        assert r.value == exp_v, key
    assert r.consumed == E[key + "_consumed"], key
    assert r.ok == E[key + "_ok"], key
    assert r.ec == E[key + "_ec"], key


def _assert_int(key, r):
    assert r.value == E[key + "_value"], key
    assert r.consumed == E[key + "_consumed"], key
    assert r.ok == E[key + "_ok"], key
    assert r.ec == E[key + "_ec"], key


# --- Layer 1: double parses, differential vs native oracle ---

def test_double_ordinary_and_trailing():
    _assert_double("d_pi", m.parse_double("3.14159 rest", G))
    _assert_double("d_trail", m.parse_double("2.5xyz", G))
    _assert_double("d_sci", m.parse_double("1.5e3", G))
    _assert_double("d_neg", m.parse_double("-0.0", G))


def test_double_error_paths():
    _assert_double("d_err", m.parse_double("nope", G))
    _assert_double("d_empty", m.parse_double("", G))


def test_double_tricky_rounding():
    # smallest positive subnormal, an exact round-to-even tie, and overflow->inf
    _assert_double("d_subnormal", m.parse_double("5e-324", G))
    _assert_double("d_tie", m.parse_double("1.0000000000000002", G))
    _assert_double("d_huge", m.parse_double("1e400", G))


def test_double_inf_nan():
    _assert_double("d_inf", m.parse_double("inf", G))
    _assert_double("d_nan", m.parse_double("nan", G))


def test_format_gating():
    # hex mode does not consume an 0x prefix; without it, exponent marker is 'p' but the
    # tail is left unconsumed -- both exactly mirrored from the oracle.
    _assert_double("h_prefix", m.parse_double("0x1.8p1", H))
    _assert_double("h_noprefix", m.parse_double("1.8p1", H))
    # scientific-only rejects fixed and vice versa
    _assert_double("s_only_ok", m.parse_double("1e10", S))
    _assert_double("s_reject_fixed", m.parse_double("12.5", S))
    _assert_double("f_only_ok", m.parse_double("12.5", F))
    _assert_double("f_reject_sci", m.parse_double("1e5", F))


def test_float_single_precision():
    _assert_double("f32_third", m.parse_float("0.1", G))
    _assert_double("f32_big", m.parse_float("3.4e38", G))


def test_options_custom_decimal_point():
    opts = m.parse_options_tChar(G, ",", 10)
    _assert_double("opt_comma", m.parse_double_opts("3,5", opts))
    _assert_double("opt_comma_dotfail", m.parse_double_opts("3.5", opts))


def test_integer_parses():
    _assert_int("i_pos", m.parse_int("12345", 10))
    _assert_int("i_neg", m.parse_int("-42xyz", 10))
    _assert_int("i_err", m.parse_int("abc", 10))
    _assert_int("i_hex", m.parse_int("ff", 16))
    _assert_int("i_bin", m.parse_int("1010", 2))


def test_unsigned_parses_and_overflow():
    _assert_int("u_dec", m.parse_uint("255 ", 16))
    _assert_int("u_big", m.parse_uint("18446744073709551615", 10))
    _assert_int("u_overflow", m.parse_uint("18446744073709551616", 10))


def test_chars_format_enum_values_differential():
    assert CF.scientific.value == E["cf_scientific"]
    assert CF.fixed.value == E["cf_fixed"]
    assert CF.hex.value == E["cf_hex"]
    assert CF.no_infnan.value == E["cf_no_infnan"]
    assert CF.general.value == E["cf_general"]
    assert CF.json.value == E["cf_json"]
    assert CF.json_or_infnan.value == E["cf_json_or_infnan"]
    assert CF.fortran.value == E["cf_fortran"]
    assert CF.allow_leading_plus.value == E["cf_allow_leading_plus"]
    assert CF.skip_white_space.value == E["cf_skip_white_space"]


# --- Layer 3: invariants on the head-on-bound library types ---

def test_parse_options_struct_roundtrip():
    # parse_options_t<char> bound head-on: ctor (fmt, dot:str, base:int) + the three fields,
    # mutable. decimal_point is a C++ `char` field surfaced as a 1-char str.
    o = m.parse_options_tChar(G, ".", 16)
    assert o.format.value == G.value
    assert o.decimal_point == "."
    assert o.base == 16
    o.base = 8
    o.decimal_point = ","
    assert o.base == 8
    assert o.decimal_point == ","


def test_from_chars_result_struct():
    # from_chars_result_t<char> bound head-on: a default-constructed instance exposes ptr
    # (a const char* -> None when null) and operator bool (the P2497 conversion). The `ec`
    # field is std::errc, which has no nanobind caster (see findings_draft); reading it is
    # expected to raise rather than silently misbehave -- asserted explicitly so the gap is
    # locked in, not hidden.
    r = m.from_chars_result_tChar()
    assert r.ptr is None
    assert bool(r) is True            # default errc() == std::errc() => operator bool true
    with pytest.raises(TypeError):
        _ = r.ec


def test_parser_surface_present():
    for fn in ("parse_double", "parse_float", "parse_int", "parse_uint",
               "parse_double_opts"):
        assert hasattr(m, fn), fn
    for ty in ("chars_format", "parse_options_tChar", "from_chars_result_tChar",
               "ParseDouble", "ParseInt", "ParseUInt"):
        assert hasattr(m, ty), ty
