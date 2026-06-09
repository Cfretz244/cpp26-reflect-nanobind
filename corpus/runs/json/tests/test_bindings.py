"""Intended correctness test for nlohmann/json bindings — currently SKIPPED.

The module does not build: binding the whole `basic_json` value type is intractable on the
pinned toolchain (exceeds the constexpr step budget; ICEs when forced — see TC-0002), so this
run's outcome is B. These assertions are preserved so that, if the toolchain/binder later make
`^^nlohmann::json` buildable, re-running gets real differential coverage for free.

Intended surface (binder-exposed, non-template): dump(), is_*() predicates, size(), operator[],
json<->json comparison; with jsontest factories/parse supplying populated json inputs and the
native oracle providing ground-truth dump() strings.
"""
import pytest

json_ext = pytest.importorskip(
    "json_ext", reason="json bindings do not build (intractable heavy type; see TC-0002)")


def test_dump_round_trip():
    m = json_ext
    j = m.jsontest.parse('{"a":1,"b":[2,3],"c":"x"}')
    assert j.dump() == '{"a":1,"b":[2,3],"c":"x"}'
    assert j.is_object()
    assert j.size() == 3


def test_scalar_predicates():
    m = json_ext
    assert m.jsontest.integer(5).is_number()
    assert m.jsontest.text("hi").is_string()
    assert m.jsontest.null_value().is_null()


def test_equality():
    m = json_ext
    assert m.jsontest.integer(7) == m.jsontest.integer(7)
    assert m.jsontest.integer(7) != m.jsontest.integer(8)
