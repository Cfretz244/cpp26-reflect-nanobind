"""Differential test for the fmtlib/fmt binding (Layer-1 differential + Layer-3 invariants).

The binder exposes the real fmt enums (`color`, `terminal_color`, `emphasis`) directly, plus a
fixture namespace `fmttest` of concrete free-function wrappers (apply_int/double/str/two, pad_left,
hex_upper, join_ints/strs) and a fixture `Formatter` class. fmt's entire public formatting API is
function templates (binder-skipped), so the wrappers are the only way to drive `fmt::format` from
Python; they are TEST SCAFFOLDING — the formatting output they return is the real fmt engine's.

Layer 1 (differential): `oracle_native.cpp` calls the SAME wrapper functions and reads the SAME
enum values natively, emitting ground truth as expected.json; every value assertion below compares
the bound module against it. Native harness and bindings share one compiler + one fmt, so any
divergence is the binding layer's. Layer 3 adds structural invariants (kwargs binding, the
member-function-template gap).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("fmt_ext")

_EXPECTED_PATH = pathlib.Path(__file__).parent / "expected.json"
if not _EXPECTED_PATH.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)",
                allow_module_level=True)
E = _json.loads(_EXPECTED_PATH.read_text())


# --- Layer 1: free-function formatting vs native fmt ground truth ---

def test_apply_format_strings():
    assert m.apply_int("{:05d}", 42) == E["apply_int_05d"]
    assert m.apply_int("{:x}", 255) == E["apply_int_hex"]
    assert m.apply_double("{:.3f}", 3.14159) == E["apply_double_f"]
    assert m.apply_str("{:>6}", "hi") == E["apply_str_r6"]
    assert m.apply_two("{0}-{1}-{0}", 7, 9) == E["apply_two"]


def test_compiletime_wrappers():
    assert m.pad_left(7, 5) == E["pad_left_7_5"]
    assert m.hex_upper(255) == E["hex_upper_255"]


def test_join_over_stl_containers():
    assert m.join_ints([1, 2, 3], ", ") == E["join_ints"]
    assert m.join_strs(["a", "b", "c"], "/") == E["join_strs"]


def test_formatter_fixture_class():
    assert m.Formatter("{:08b}").apply_int(5) == E["fmtr_bin"]
    assert m.Formatter().spec() == E["fmtr_spec"]
    assert m.Formatter("{:#x}").apply_int(255) == E["fmtr_hash_hex"]


def test_real_fmt_to_string_instantiations():
    # Bound DIRECTLY from fmt's OWN to_string<T> template instantiations (^^fmt::to_string<int>,
    # etc.) — not a wrapper. Names carry the template-spec CamelCase + enable_if-NTTP suffix
    # (BINDER-0003 cosmetic). Each formats a real value through fmt; checked vs native fmt.
    assert m.to_stringInt0(42) == E["ts_int_42"]
    assert m.to_stringInt0(-7) == E["ts_int_neg"]
    assert m.to_stringDouble0(2.5) == E["ts_double_2_5"]
    assert m.to_stringLonglong0(9999999999) == E["ts_ll_big"]


def test_real_fmt_format_int_class():
    # Bound DIRECTLY from fmt's real concrete class fmt::format_int (its fast int->string
    # formatter): constructor overloads + str()/c_str()/size() all bind; checked vs native fmt.
    assert m.format_int(123456).str() == E["fi_123456"]
    assert m.format_int(-42).str() == E["fi_neg"]
    assert m.format_int(123456).size() == E["fi_size"]
    assert m.format_int(123456).c_str() == E["fi_123456"]
    # private member templates (format_signed/format_unsigned) are not bound
    assert not hasattr(m.format_int, "format_signed")


def test_real_fmt_enum_values():
    assert m.color.red.value == E["color_red"]
    assert m.color.alice_blue.value == E["color_alice_blue"]
    assert m.terminal_color.red.value == E["term_red"]
    assert m.emphasis.bold.value == E["emph_bold"]
    assert m.emphasis.italic.value == E["emph_italic"]


# --- Layer 3: structural invariants (independent of the oracle) ---

def test_kwargs_bound():
    # P3096 parameter names -> nb::arg: these wrappers accept Python keyword arguments,
    # in any order. An unknown keyword is rejected, proving the names are really bound.
    assert m.apply_int(spec="{:03d}", value=7) == "007"
    assert m.apply_int(value=7, spec="{:03d}") == "007"
    assert m.pad_left(value=9, width=4) == "   9"
    assert m.pad_left(width=4, value=9) == "   9"
    with pytest.raises(TypeError):
        m.pad_left(value=9, nope=4)


def test_member_function_template_gap():
    # The fixture class binds ordinary members but the binder skips member function templates.
    assert hasattr(m.Formatter, "apply_int")   # ordinary method -> bound
    assert hasattr(m.Formatter, "spec")        # ordinary method -> bound
    assert not hasattr(m.Formatter, "apply")   # member template apply<T> -> skipped


def test_enum_names_and_scale():
    # The real fmt::color enum binds with its CSS-color enumerator names intact.
    assert m.color.red.name == "red"
    assert m.color.alice_blue.name == "alice_blue"
    for name in ("black", "white", "blue", "green", "yellow", "magenta", "cyan"):
        assert hasattr(m.terminal_color, name)
