"""Differential test for the Abseil strings binding (Layer-1 differential + Layer-3).

absl::Cord binds head-on: explicit Cord(string_view) ctor, Append/Prepend (non-template
overloads via the string_view caster), RemovePrefix/RemoveSuffix, Subcord, size/empty/
Clear, Compare/StartsWith/EndsWith, operator<< -> __str__. The variadic-template free
functions (StrCat/StrJoin/StrSplit/SimpleAtoi/ascii) go through the strtest fixture
wrappers. oracle_native.cpp drives the identical surface natively.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_strings_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def test_cord_append_differential():
    c = m.Cord("hello ")
    c.Append("world")
    assert c.size() == E["cord_size"]
    assert m.cord_str(c) == E["cord_str"]
    assert str(c) == E["cord_stream"]          # operator<< -> __str__


def test_cord_prepend_remove_differential():
    c = m.Cord("hello ")
    c.Append("world")
    c.Prepend(">> ")
    assert m.cord_str(c) == E["cord_prepended"]
    c.RemovePrefix(3)
    assert m.cord_str(c) == E["cord_rm_prefix"]
    c.RemoveSuffix(5)
    assert m.cord_str(c) == E["cord_rm_suffix"]


def test_cord_query_differential():
    full = m.Cord("hello world")
    assert m.cord_str(full.Subcord(6, 5)) == E["cord_subcord"]
    assert full.StartsWith("hello") == E["cord_starts"]
    assert full.EndsWith("world") == E["cord_ends"]
    assert full.Compare(m.Cord("hello world")) == E["cord_cmp_eq"]
    assert m.Cord("apple").Compare(m.Cord("banana")) == E["cord_cmp_lt"]


def test_cord_clear_differential():
    c = m.Cord("x")
    c.Clear()
    assert c.empty() == E["cord_cleared_empty"]


def test_strcat_differential():
    assert m.cat2("foo", "bar") == E["cat2"]
    assert m.cat3("a", "-", "b") == E["cat3"]
    assert m.cat_int("n=", 42) == E["cat_int"]


def test_join_split_differential():
    assert m.join(["a", "b", "c"], "-") == E["join"]
    parts = m.split("x,y,z", ",")
    assert len(parts) == E["split_n"]
    assert parts[1] == E["split_1"]


def test_numbers_ascii_differential():
    assert m.to_int("12345") == E["to_int_ok"]
    assert (m.to_int("not a number") is None) == E["to_int_bad_is_none"]
    assert m.upper("MixedCase42") == E["upper"]
    assert m.lower("MixedCase42") == E["lower"]


# --- Layer 3: structural invariants ---

def test_cord_invariants():
    c = m.Cord()                  # default ctor
    assert c.empty() and c.size() == 0
    c.Append(m.Cord("ab"))        # Append(const Cord&) overload
    c.Append("cd")                # Append(string_view) overload
    assert m.cord_str(c) == "abcd"
