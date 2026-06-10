"""Differential test for the tl::expected binding (Layer-1 + Layer-3).

Three real expected<T,E> specializations (int/std::string, the role-swapped
std::string/int, and the no-value void/std::string) are bound head-on;
unexpected<str>/<int> and bad_expected_access<str> directly; extest supplies
factories (the converting ctors are templates) and thin wrappers over the
all-template operator== / value_or / monadic fronts. oracle_native.cpp drives
the IDENTICAL scenarios natively and emits every observable; the assertions
here compare the bound module against that ground truth, plus Layer-3
structure (spec naming, the real C++ throw path surfacing as RuntimeError,
deleted-ctor TypeError).

This surface formerly sat at outcome B behind TC-0008 + BINDER-0012, with the
void spec dropped for TC-0006/TC-0007 and value() silently unbound by TC-0009
(found by this run's first post-fix execution); all five are fixed and the
run is green end-to-end.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("expected_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# CamelCase spec names (spec_camel_name): base identifier keeps its case, std::string
# renders as String.
ExpInt = m.expectedIntString          # tl::expected<int, std::string>
ExpStr = m.expectedStringInt          # tl::expected<std::string, int>
ExpVoid = m.expectedVoidString        # tl::expected<void, std::string>
UnexpStr = m.unexpectedString         # tl::unexpected<std::string>
UnexpInt = m.unexpectedInt            # tl::unexpected<int>
BadAccess = m.bad_expected_accessString


# --- Layer 1: differential vs the native oracle ---

def test_default_ctor_differential():
    di = ExpInt()
    assert di.has_value() == E["def_int_has"]
    assert di.value() == E["def_int_value"]
    ds = ExpStr()
    assert ds.has_value() == E["def_str_has"]
    assert ds.value() == E["def_str_value"]


def test_ok_err_observers_differential():
    ok = m.ok_int(42)
    assert ok.has_value() == E["ok_has"]
    assert bool(ok) == E["ok_bool"]          # explicit operator bool -> __bool__
    assert ok.value() == E["ok_value"]
    err = m.err_int("nope")
    assert err.has_value() == E["err_has"]
    assert bool(err) == E["err_bool"]
    assert err.error() == E["err_error"]


def test_throwing_value_differential():
    # value() on an errored expected throws the REAL bad_expected_access<string>;
    # nanobind surfaces any std::exception as RuntimeError carrying what().
    err = m.err_int("nope")
    with pytest.raises(RuntimeError) as ei:
        err.value()
    assert str(ei.value) == E["throw_what"]


def test_swap_differential():
    a = m.ok_int(7)
    b = m.err_int("swapped")
    a.swap(b)
    assert a.has_value() == E["swap_a_has"]
    assert a.error() == E["swap_a_error"]
    assert b.has_value() == E["swap_b_has"]
    assert b.value() == E["swap_b_value"]


def test_copy_ctor_differential():
    ok = m.ok_int(42)
    copy = ExpInt(ok)
    assert copy.has_value() == E["copy_has"]
    assert copy.value() == E["copy_value"]


def test_unexpected_differential():
    # unexpected<E>'s REAL non-template ctor: a Python str/int constructs one.
    u = UnexpStr("boom")
    assert u.value() == E["unexp_value"]
    ui = UnexpInt(17)
    assert ui.value() == E["unexp_int_value"]
    # ... and crosses back into the real expected(const unexpected<G>&) ctor.
    ue = m.from_unexpected(u)
    assert ue.has_value() == E["from_unexp_has"]
    assert ue.error() == E["from_unexp_error"]


def test_role_swapped_spec_differential():
    os_ = m.ok_str("hello expected")
    assert os_.has_value() == E["ok_str_has"]
    assert os_.value() == E["ok_str_value"]
    es = m.err_str(404)
    assert es.has_value() == E["err_str_has"]
    assert es.error() == E["err_str_error"]


def test_value_or_and_equality_differential():
    ok, err = m.ok_int(42), m.err_int("nope")
    assert m.value_or_int(ok, -1) == E["value_or_ok"]
    assert m.value_or_int(err, -1) == E["value_or_err"]
    assert m.eq_int(m.ok_int(1), m.ok_int(1)) == E["eq_ok_ok"]
    assert m.eq_int(m.ok_int(1), m.err_int("x")) == E["eq_ok_err"]
    assert m.eq_int(m.err_int("x"), m.err_int("x")) == E["eq_err_err"]
    assert m.eq_int_value(ok, 42) == E["eq_value"]
    assert m.eq_int_unexpected(err, UnexpStr("nope")) == E["eq_unexp"]


def test_monadic_wrappers_differential():
    assert m.map_double(m.ok_int(21)).value() == E["map_ok"]
    assert m.map_double(m.err_int("m")).error() == E["map_err"]
    assert m.and_then_halve(m.ok_int(10)).value() == E["and_then_even"]
    assert m.and_then_halve(m.ok_int(3)).error() == E["and_then_odd"]
    assert m.and_then_halve(m.err_int("early")).error() == E["and_then_early"]


def test_bad_expected_access_differential():
    bea = BadAccess("direct")
    assert bea.error() == E["bea_error"]
    assert bea.what() == E["bea_what"]


def test_void_spec_differential():
    # expected<void, std::string>: the no-value success channel. Reflecting
    # this spec used to wrong-reject (TC-0007) and SEGV the compiler (TC-0006).
    vd = ExpVoid()
    assert vd.has_value() == E["void_def_has"]
    assert bool(vd) == E["void_def_bool"]
    vok = m.ok_void()
    assert vok.has_value() == E["void_ok_has"]
    verr = m.err_void("void failed")
    assert verr.has_value() == E["void_err_has"]
    assert bool(verr) == E["void_err_bool"]
    assert verr.error() == E["void_err_error"]
    vcopy = ExpVoid(verr)
    assert vcopy.has_value() == E["void_copy_has"]
    assert vcopy.error() == E["void_copy_error"]


# --- Layer 3: invariants ---

def test_factories_return_bound_spec_types():
    assert type(m.ok_int(1)) is ExpInt
    assert type(m.err_int("e")) is ExpInt
    assert type(m.from_unexpected(UnexpStr("u"))) is ExpInt
    assert type(m.ok_str("s")) is ExpStr
    assert type(m.map_double(m.ok_int(2))) is ExpInt


def test_deleted_default_ctor_not_bound():
    # unexpected() = delete -- Python construction without the error value must
    # raise TypeError, not invoke a deleted constructor.
    with pytest.raises(TypeError):
        UnexpStr()
    with pytest.raises(TypeError):
        UnexpInt()


def test_expected_surface_bound():
    for meth in ("has_value", "value", "error", "swap"):
        assert hasattr(ExpInt, meth), meth
        assert hasattr(ExpStr, meth), meth
    assert hasattr(ExpInt, "__bool__")


def test_unbindable_surface_absent():
    # Member templates with non-defaulted parameters must NOT appear.
    for meth in ("value_or", "and_then", "map", "transform", "or_else", "emplace"):
        assert not hasattr(ExpInt, meth), meth


def test_void_spec_surface():
    # value<U=T>() and swap<OT=T,...>() are enable_if'd away for T=void: with
    # TC-0006 fixed, can_substitute reports not-substitutable (instead of
    # crashing) and the binder skips them -- they must be absent, while the
    # void-valid surface is present.
    for meth in ("has_value", "error"):
        assert hasattr(ExpVoid, meth), meth
    assert hasattr(ExpVoid, "__bool__")
    for meth in ("value", "swap"):
        assert not hasattr(ExpVoid, meth), meth
