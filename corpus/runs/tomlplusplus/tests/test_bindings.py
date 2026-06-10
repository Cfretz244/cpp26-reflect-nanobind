"""Differential test for the toml++ binding (Layer-1 + Layer-3).

toml++'s date/time value structs, node_type enum, and parse_error/source_region/
source_position are bound HEAD-ON; the tomlfix namespace (bound as free functions) parses +
walks the document tree with the library's own code (the tree's node/table/array classes are
unbindable -- see findings_draft/binder-lvalue-ref-to-pointer-param.md) and returns plain
Python values plus those real bound types. oracle_native.cpp drives the EXACT same TOML
document natively through the same library and emits every observable; the assertions below
compare the bound module against that ground truth value-for-value, then add Layer-3
structural invariants on the real bound types.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("tomlpp_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# The exact document the native oracle parsed (kept byte-identical here).
DOC = (
    'title = "TOML Example"\n'
    'enabled = true\n'
    'pi = 3.14159\n'
    'count = 42\n'
    'neg = -7\n'
    'big = 9223372036854775807\n'
    'name = "café"\n'
    'ratio = 1.5e3\n'
    'the_date = 2024-03-16\n'
    'the_time = 13:45:06\n'
    'local_dt = 1979-05-27T07:32:00\n'
    'offset_dt = 1979-05-27T07:32:00-07:00\n'
    'ints = [1, 2, 3, 5, 8]\n'
    'mixed = [10, "two", true]\n'
    '[server]\n'
    'host = "localhost"\n'
    'port = 8080\n'
    '[server.limits]\n'
    'max = 1000\n'
)


# ---------- Layer 1: differential vs the native oracle ----------

def test_parse_and_shape_differential():
    assert m.parse_ok(DOC) == E["parse_ok"]
    assert m.table_size(DOC) == E["table_size"]
    assert m.top_keys(DOC) == E["top_keys"]


def test_node_type_differential():
    # node_type is the real bound enum; compare its .value to the native int.
    assert m.type_of(DOC, "count").value == E["ntype_count"]
    assert m.type_of(DOC, "pi").value == E["ntype_pi"]
    assert m.type_of(DOC, "enabled").value == E["ntype_enabled"]
    assert m.type_of(DOC, "name").value == E["ntype_name"]
    assert m.type_of(DOC, "the_date").value == E["ntype_the_date"]
    assert m.type_of(DOC, "the_time").value == E["ntype_the_time"]
    assert m.type_of(DOC, "local_dt").value == E["ntype_local_dt"]
    assert m.type_of(DOC, "ints").value == E["ntype_ints"]
    assert m.type_of(DOC, "server").value == E["ntype_server"]


def test_coerced_scalars_differential():
    assert m.get_int(DOC, "count") == E["count"]
    assert m.get_int(DOC, "neg") == E["neg"]
    assert m.get_int(DOC, "big") == E["big"]
    assert m.get_float(DOC, "pi") == pytest.approx(E["pi"])
    assert m.get_float(DOC, "ratio") == pytest.approx(E["ratio"])
    assert m.get_bool(DOC, "enabled") == E["enabled"]
    assert m.get_string(DOC, "name") == E["name"]
    assert m.get_int(DOC, "server.port") == E["nested_port"]
    assert m.get_int(DOC, "server.limits.max") == E["deep_max"]
    assert m.get_string(DOC, "server.host") == E["nested_host"]
    # permissive retrieval: an int request on a float node yields None (== nullopt)
    assert (m.get_int(DOC, "pi") is None) == E["int_of_pi_empty"]


def test_date_time_values_differential():
    d = m.get_date(DOC, "the_date")
    assert isinstance(d, m.date)
    assert [d.year, d.month, d.day] == [E["date_year"], E["date_month"], E["date_day"]]
    tm = m.get_time(DOC, "the_time")
    assert isinstance(tm, m.time)
    assert [tm.hour, tm.minute, tm.second] == [E["time_hour"], E["time_min"], E["time_sec"]]
    ldt = m.get_datetime(DOC, "local_dt")
    assert isinstance(ldt, m.date_time)
    assert ldt.date.year == E["ldt_year"]
    assert ldt.time.hour == E["ldt_hour"]
    assert ldt.is_local() == E["ldt_is_local"]
    odt = m.get_datetime(DOC, "offset_dt")
    assert odt.is_local() == E["odt_is_local"]
    assert odt.offset.minutes == E["odt_offset_min"]


def test_arrays_differential():
    assert m.array_size(DOC, "ints") == E["ints_size"]
    assert m.int_array(DOC, "ints") == E["ints_vals"]
    assert m.array_size(DOC, "mixed") == E["mixed_size"]
    # array_types is a list of the real node_type enum; compare the int values
    nt = m.node_type
    name_of = {nt.integer.value: "integer", nt.string.value: "string",
               nt.boolean.value: "boolean"}
    got = [name_of[t.value] for t in m.array_types(DOC, "mixed")]
    assert got == E["mixed_types"]


def test_serialization_roundtrip_differential():
    assert m.to_toml(DOC) == E["toml_roundtrip"]
    assert m.to_json(DOC) == E["json_roundtrip"]


def test_error_path_differential():
    bad = "x = = 3\n"
    assert m.parse_ok(bad) == E["bad_parse_ok"]
    err = m.parse_error_of(bad)
    assert isinstance(err, m.parse_error)            # the real bound error type
    assert err.description() == E["err_desc"]
    src = err.source()                               # real source_region
    assert isinstance(src, m.source_region)
    assert src.begin.line == E["err_line"]           # real source_position
    assert src.begin.column == E["err_col"]


# ---------- Layer 3: structural invariants on the real bound types ----------

def test_node_type_enum_surface():
    nt = m.node_type
    for name in ("none", "table", "array", "string", "integer",
                 "floating_point", "boolean", "date", "time", "date_time"):
        assert hasattr(nt, name)


def test_value_structs_are_real_bound_types():
    # constructible + read/write data members (real toml++ structs, not stand-ins)
    d = m.date(); d.year = 2030; d.month = 12; d.day = 31
    assert (d.year, d.month, d.day) == (2030, 12, 31)
    tm = m.time(); tm.hour = 23; tm.minute = 59; tm.second = 58; tm.nanosecond = 5
    assert (tm.hour, tm.minute, tm.second, tm.nanosecond) == (23, 59, 58, 5)
    off = m.time_offset(); off.minutes = -300
    assert off.minutes == -300


def test_date_fields_match_parsed():
    # toml::date's comparison operators are hidden friends (in-class friend operator==,
    # operator<, ...), which are NOT namespace members and so are not reflected/bound --
    # a reflection limitation, not a value bug. The bound surface is the data members; a
    # round-trip rebuild from a parsed date reproduces the same fields exactly.
    a = m.get_date(DOC, "the_date")
    b = m.date(); b.year = a.year; b.month = a.month; b.day = a.day
    assert (b.year, b.month, b.day) == (a.year, a.month, a.day)
    # no bound ordering operator: a < b raises (default object has no <)
    with pytest.raises(TypeError):
        _ = a < b
