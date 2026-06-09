"""Differential test for the REAL nlohmann::json binding (Layer-1 + Layer-3).

The binder exposes `nlohmann::basic_json` itself (no facade): its `is_*()`
predicates, `size()`, `operator[]`, `operator==`, `dump()`, plus the real
`detail::value_t` / `detail::error_handler_t` enums. The `jsontest` free functions
are TEST FIXTURES -- they build/extract values through json's member-templated
(hence binder-skipped) C++ API so Python has populated json objects to exercise the
binder-exposed surface.

Layer 1 (differential): `oracle_native.cpp` runs the SAME operations through
nlohmann::json natively and emits ground truth as expected.json; every assertion
below compares the bound module's result against that ground truth. Native harness
and bindings share one compiler and one json, so any divergence is the binding
layer's. Layer 3 adds structural invariants.

json's binary-format internals (output_adapter family, byte_container_with_subtype,
iter_impl) are marked `[[=reflect::skip]]` in the json test copy -- implementation
detail, not cleanly bindable. Everything tested is the real, unmodified public API.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("json_ext")

_EXPECTED_PATH = pathlib.Path(__file__).parent / "expected.json"
if not _EXPECTED_PATH.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)",
                allow_module_level=True)
E = _json.loads(_EXPECTED_PATH.read_text())


# json's dump() default-argument *values* aren't bindable (a C++26 standard gap),
# so pass them explicitly: compact, space indent char, no escaping, strict.
def _dump(j):
    return j.dump(-1, " ", False, m.error_handler_t.strict)


def test_object_parse_and_dump():
    j = m.parse('{"a":1,"b":[2,3],"c":"x"}')
    assert _dump(j) == E["obj_dump"]        # vs native ground truth
    assert j.is_object() == E["obj_is_object"]
    assert j.size() == E["obj_size"]


def test_subscript_returns_real_json():
    j = m.parse('{"a":1,"b":[2,3],"c":"x"}')
    assert _dump(j["b"]) == E["b_dump"]
    assert j["b"].is_array() == E["b_is_array"]
    assert j["b"].size() == E["b_size"]
    assert j["a"].is_number() == E["a_is_number"]
    assert j["c"].is_string() == E["c_is_string"]


def test_scalar_dumps():
    assert _dump(m.integer(0)) == E["int0_dump"]
    assert _dump(m.text("hi")) == E["text_dump"]
    assert _dump(m.boolean(True)) == E["bool_dump"]
    assert _dump(m.null_value()) == E["null_dump"]
    assert _dump(m.array_123()) == E["arr123_dump"]


def test_equality():
    assert (m.integer(7) == m.integer(7)) == E["eq_7_7"]
    assert (m.integer(7) == m.integer(8)) == E["eq_7_8"]


def test_value_extraction_round_trip():
    assert m.to_int(m.integer(42)) == E["to_int_42"]
    assert m.to_text(m.text("hello")) == E["to_text_hello"]
    assert m.to_double(m.number(2.5)) == E["to_double_2_5"]


# --- Layer 3: structural invariants (independent of the oracle) ---

def test_predicate_invariants():
    assert m.integer(5).is_number()
    assert m.text("hi").is_string()
    assert m.null_value().is_null()
    assert m.boolean(True).is_boolean()
    assert m.array_123().is_array()


def test_value_t_enum_bound():
    for name in ("null", "object", "array", "string", "boolean",
                 "number_integer", "number_unsigned", "number_float"):
        assert hasattr(m.value_t, name)
