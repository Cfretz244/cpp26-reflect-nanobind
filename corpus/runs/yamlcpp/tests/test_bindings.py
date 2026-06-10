"""Differential test for the yaml-cpp 0.9.0 binding (Layer-1 + Layer-3).

The binder binds YAML::Node head-on (Type/IsNull/IsScalar/IsSequence/IsMap/IsDefined/size/
Scalar/Tag/SetTag/Style/SetStyle/reset/is, __bool__, __getitem__), the YAML::NodeType::value
enum (m.value), YAML::Mark, and the Dump/Clone free functions. The yamlfix fixture forwards the
member-template-only accessors (Node::as<T>, Node::operator[](Key)) and the overloaded Load.

oracle_native.cpp parses the SAME fixed YAML document natively and emits its observable
properties; these assertions drive that same document through the bound module and compare
(shared compiler + shared yaml-cpp lib).
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("yamlcpp_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

DOC = (
    "name: Sam\n"
    "age: 42\n"
    "ratio: 3.5\n"
    "flag: true\n"
    "items:\n"
    "  - a\n"
    "  - b\n"
    "  - c\n"
    "nested:\n"
    "  x: 1\n"
    "  y: 2\n"
)

NT = m.value  # NodeType::value enum (Undefined/Null/Scalar/Sequence/Map)


@pytest.fixture
def root():
    return m.load(DOC)


# --- Layer 1: differential vs the native oracle ---

def test_root_node_differential(root):
    assert root.Type().value == E["root_type"]
    assert root.Type() == NT.Map
    assert root.size() == E["root_size"]
    assert root.IsMap() is True
    assert root.IsMap() == (E["root_is_map"] is True)
    assert root.IsDefined() == (E["root_defined"] is True)


def test_scalar_node_and_coercions(root):
    name = m.get_key(root, "name")
    assert name.Type().value == E["name_type"]
    assert name.Type() == NT.Scalar
    assert name.IsScalar() == (E["name_is_scalar"] is True)
    assert name.Scalar() == E["name_scalar"]
    assert m.as_str(name) == E["name_as_str"]
    assert m.as_int(m.get_key(root, "age")) == E["age_int"]
    assert m.as_double(m.get_key(root, "ratio")) == E["ratio_dbl"]
    assert m.as_bool(m.get_key(root, "flag")) == (E["flag_bool"] is True)


def test_sequence_node(root):
    items = m.get_key(root, "items")
    assert items.Type().value == E["items_type"]
    assert items.Type() == NT.Sequence
    assert items.size() == E["items_size"]
    assert m.as_str(m.get_idx(items, 0)) == E["items_0"]
    assert m.as_str(m.get_idx(items, 2)) == E["items_2"]


def test_nested_map_node(root):
    nested = m.get_key(root, "nested")
    assert nested.Type().value == E["nested_type"]
    assert nested.Type() == NT.Map
    assert m.as_int(m.get_key(nested, "x")) == E["nested_x"]
    assert m.as_int(m.get_key(nested, "y")) == E["nested_y"]


def test_missing_key_undefined(root):
    missing = m.get_key(root, "does_not_exist")
    assert missing.IsDefined() == (E["missing_defined"] is True)
    assert missing.IsDefined() is False
    assert bool(missing) is False           # Node::operator bool -> __bool__
    assert m.has_key(root, "name") is True
    assert m.has_key(root, "does_not_exist") is False


def test_nodetype_enum_values():
    assert NT.Undefined.value == E["nt_undefined"]
    assert NT.Null.value == E["nt_null"]
    assert NT.Scalar.value == E["nt_scalar"]
    assert NT.Sequence.value == E["nt_sequence"]
    assert NT.Map.value == E["nt_map"]


def test_dump_roundtrip_byte_for_byte(root):
    assert m.Dump(root) == E["dump"]


def test_clone_dumps_identically(root):
    clone = m.Clone(root)
    assert m.Dump(clone) == E["clone_dump"]
    assert m.Dump(clone) == m.Dump(root)


def test_fallback_coercion_swallows_badconversion(root):
    ht = m.load("k: hello")
    assert m.as_int_or(m.get_key(ht, "k"), 99) == E["fallback_int"]
    assert m.as_str_or(m.get_key(ht, "missingkey"), "DEF") == E["fallback_str"]


def test_malformed_input_error_path():
    # yaml-cpp's ParserException; nanobind's default translator surfaces it as RuntimeError
    # carrying what(). Assert the kind the oracle caught + message equality.
    assert E["parse_err_kind"] == "ParserException"
    with pytest.raises(RuntimeError) as ei:
        m.load("{ unclosed: [1, 2, 3")
    assert str(ei.value) == E["parse_err_what"]


def test_bad_conversion_error_path():
    assert E["badconv_kind"] == "BadConversion"
    ht = m.load("k: hello")
    with pytest.raises(RuntimeError) as ei:
        m.as_int(m.get_key(ht, "k"))
    assert str(ei.value) == E["badconv_what"]


# --- Layer 3: invariants (surface present, types) ---

def test_node_surface_bound():
    for meth in ("Type", "IsNull", "IsScalar", "IsSequence", "IsMap", "IsDefined",
                 "size", "Scalar", "Tag", "SetTag", "Style", "SetStyle", "reset", "is_"):
        # nanobind renames the C++ 'is' method? check both names
        if meth == "is_":
            continue
        assert hasattr(m.Node, meth), meth


def test_mark_surface_bound():
    for attr in ("pos", "line", "column", "is_null", "null_mark"):
        assert hasattr(m.Mark, attr), attr


def test_node_type_is_real_enum():
    n = m.Node()
    assert isinstance(n.Type(), NT)         # default Node Type() is a NodeType enum instance
    assert n.Type() == NT.Null
