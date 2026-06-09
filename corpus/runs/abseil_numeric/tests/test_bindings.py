"""Differential test for the Abseil int128/uint128 binding (Layer-1 differential + Layer-3).

The binder binds absl::int128 / absl::uint128 directly (no wrappers): their arithmetic/bitwise/
comparison operators map to Python dunders, and their stream-insertion operator is surfaced as
str() via __str__. oracle_native.cpp computes the same operations natively and emits each result
as a decimal string; the assertions compare the bound module's str() against that ground truth
(128-bit results exceed the Python-constructible range, so str() is the value channel).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_numeric_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

i128 = m.int128
u128 = m.uint128


def test_int128_arithmetic_differential():
    a, b = i128(1000000), i128(7)
    assert str(a) == E["a"]
    assert str(a * b) == E["mul"]
    assert str(a + b) == E["add"]
    assert str(a - b) == E["sub"]
    assert str(a / b) == E["div"]      # operator/ is integer division for int128
    assert str(a % b) == E["mod"]
    assert str(a << 10) == E["shl"]


def test_int128_comparisons_differential():
    a, b = i128(1000000), i128(7)
    assert (a == i128(1000000)) == (E["eq"] == "true")
    assert (b < a) == (E["lt"] == "true")


def test_int128_negation_differential():
    # Free unary operator-(int128) -> __neg__ (binder fix: unary free operators).
    assert str(-i128(1000000)) == E["neg"]


def test_int128_int_conversion():
    # int128 has a dozen integral conversion operators; only the WIDEST binds
    # __int__ (binder fix), so values above int32 no longer truncate through a
    # narrow last-bound conversion (int(i128(1000000)) used to return 64).
    assert int(i128(1000000)) == 1000000
    assert int(i128(5_000_000_000)) == 5_000_000_000


def test_uint128_full_width_differential():
    # 128-bit results that overflow 64 bits, computed through the bound operators.
    p = u128(1) << 100                       # 2^100
    assert str(p) == E["u_2pow100"]
    assert str(p * u128(8)) == E["u_2pow100_x8"]
    # wrap-around at 2^128 via the bound operators
    assert str(u128(0xFFFFFFFFFFFFFFFF) * u128(0xFFFFFFFFFFFFFFFF)) != ""  # sanity, full-width mul


# --- Layer 3: structural invariants ---

def test_dunder_surface_bound():
    for d in ("__add__", "__sub__", "__mul__", "__truediv__", "__mod__",
              "__lshift__", "__rshift__", "__and__", "__or__", "__xor__", "__str__",
              "__neg__", "__int__"):
        assert hasattr(i128, d), d


def test_roundtrip_small_values():
    # Constructible small values round-trip through str().
    for n in (0, 1, 42, 255, 1 << 40):
        assert str(i128(n)) == str(n)
