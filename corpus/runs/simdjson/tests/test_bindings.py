# Differential + invariant tests for the simdjson DOM binding.
#
# Layer 1: parse the EXACT document the native oracle parsed, drive the SAME navigation/coercion/
# discrimination sequence through the bound module, and assert equality against tests/expected.json
# (the oracle's stdout).
# Layer 3: surface present, enum values, exception types on error paths.
#
# LIFETIME: a simdjson DOM element aliases buffers owned by its parser. The Doc fixture owns the
# parser + source, so the bound Doc instance MUST stay alive while any element/array/object handle
# derived from it is in use. The `doc` fixture keeps the Doc; tests take root() off it each time.
import json
import math
import os

import pytest

import simdjson_ext as sj

HERE = os.path.dirname(__file__)
with open(os.path.join(HERE, "expected.json")) as f:
    EXPECTED = json.load(f)

# Verbatim mirror of kDoc in oracle_native.cpp.
DOC = (
    "{"
    '"name": "central",'
    '"open": true,'
    '"closed": false,'
    '"count": 3,'
    '"big": 9999999999,'
    '"price": 19.5,'
    '"nothing": null,'
    '"tags": ["a", "b", "c"],'
    '"nums": [10, 20, 30],'
    '"meta": {"id": 7, "label": "x"}'
    "}"
)


@pytest.fixture(scope="module")
def doc():
    # Hold the Doc for the whole module: every element derived from it aliases its parser buffers.
    d = sj.Doc(DOC)
    assert d.ok()
    return d


# ---------------- Layer 3: surface / enum invariants ----------------

def test_surface_present():
    for name in ("Doc", "element", "array", "object", "element_type", "error_code"):
        assert hasattr(sj, name), name


def test_element_type_enum_values():
    # element_type is a char-valued enum class; values are the ASCII codes of the tag chars.
    assert sj.element_type.ARRAY.value == ord("[")
    assert sj.element_type.OBJECT.value == ord("{")
    assert sj.element_type.INT64.value == ord("l")
    assert sj.element_type.UINT64.value == ord("u")
    assert sj.element_type.DOUBLE.value == ord("d")
    assert sj.element_type.STRING.value == ord('"')
    assert sj.element_type.BOOL.value == ord("t")
    assert sj.element_type.NULL_VALUE.value == ord("n")
    assert sj.element_type.BIGINT.value == ord("Z")


def test_error_code_enum_values():
    # error_code is a plain (int) enum; SUCCESS is 0 and the named codes are present.
    assert sj.error_code.SUCCESS.value == 0
    for name in ("NO_SUCH_FIELD", "INCORRECT_TYPE", "INDEX_OUT_OF_BOUNDS"):
        assert hasattr(sj.error_code, name), name


def test_isinstance(doc):
    root = doc.root()
    assert isinstance(root, sj.element)
    assert isinstance(sj.as_array(sj.at_key(root, "tags")), sj.array)
    assert isinstance(sj.as_object(sj.at_key(root, "meta")), sj.object)


# ---------------- Layer 1: differential vs the native oracle ----------------

def test_root_discriminants(doc):
    root = doc.root()
    assert root.type().value == EXPECTED["root_type"]
    assert root.is_object() == EXPECTED["root_is_object"]
    assert sj.as_object(root).size() == EXPECTED["root_size"]


def test_scalar_reads(doc):
    root = doc.root()
    assert sj.as_string(sj.at_key(root, "name")) == EXPECTED["name"]
    assert sj.as_bool(sj.at_key(root, "open")) == EXPECTED["open"]
    assert sj.as_bool(sj.at_key(root, "closed")) == EXPECTED["closed"]
    assert sj.as_int64(sj.at_key(root, "count")) == EXPECTED["count"]
    assert sj.as_uint64(sj.at_key(root, "big")) == EXPECTED["big"]
    assert math.isclose(sj.as_double(sj.at_key(root, "price")),
                        float(EXPECTED["price"]), rel_tol=1e-12)
    assert sj.at_key(root, "nothing").is_null() == EXPECTED["nothing_is_null"]


def test_per_value_types(doc):
    root = doc.root()
    assert sj.at_key(root, "name").type().value == EXPECTED["type_name"]
    assert sj.at_key(root, "open").type().value == EXPECTED["type_open"]
    assert sj.at_key(root, "count").type().value == EXPECTED["type_count"]
    assert sj.at_key(root, "big").type().value == EXPECTED["type_big"]
    assert sj.at_key(root, "price").type().value == EXPECTED["type_price"]
    assert sj.at_key(root, "nothing").type().value == EXPECTED["type_nothing"]
    assert sj.at_key(root, "tags").type().value == EXPECTED["type_tags"]
    assert sj.at_key(root, "meta").type().value == EXPECTED["type_meta"]


def test_is_number(doc):
    root = doc.root()
    assert sj.at_key(root, "count").is_number() == EXPECTED["count_is_number"]
    assert sj.at_key(root, "price").is_number() == EXPECTED["price_is_number"]
    assert sj.at_key(root, "name").is_number() == EXPECTED["name_is_number"]
    assert sj.at_key(root, "count").is_int64() == EXPECTED["count_is_int64"]
    assert sj.at_key(root, "price").is_double() == EXPECTED["price_is_double"]


def test_arrays(doc):
    root = doc.root()
    tags = sj.as_array(sj.at_key(root, "tags"))
    assert tags.size() == EXPECTED["tags_size"]
    joined = ",".join(sj.as_string(e) for e in sj.array_values(tags))
    assert joined == EXPECTED["tags_joined"]
    assert sj.as_string(sj.array_at(tags, 1)) == EXPECTED["tags_at1"]

    nums = sj.as_array(sj.at_key(root, "nums"))
    total = sum(sj.as_int64(e) for e in sj.array_values(nums))
    assert total == EXPECTED["nums_sum"]
    assert sj.as_int64(sj.array_at(nums, 2)) == EXPECTED["nums_at2"]


def test_nested_object(doc):
    root = doc.root()
    meta = sj.as_object(sj.at_key(root, "meta"))
    assert meta.size() == EXPECTED["meta_size"]
    assert sj.as_int64(sj.object_at(meta, "id")) == EXPECTED["meta_id"]
    assert sj.as_string(sj.object_at(meta, "label")) == EXPECTED["meta_label"]
    assert ",".join(sj.object_keys(meta)) == EXPECTED["meta_keys"]


def test_json_pointer(doc):
    root = doc.root()
    assert sj.as_int64(sj.at_pointer(root, "/nums/1")) == EXPECTED["ptr_nums1"]
    assert sj.as_string(sj.at_pointer(root, "/meta/label")) == EXPECTED["ptr_metalabel"]


def test_root_keys(doc):
    obj = sj.as_object(doc.root())
    assert ",".join(sj.object_keys(obj)) == EXPECTED["root_keys"]


# ---------------- error paths: exceptions on failure ----------------

def test_missing_key_raises(doc):
    with pytest.raises(Exception):
        sj.at_key(doc.root(), "nope")


def test_wrong_type_coercion_raises(doc):
    # "name" is a string; coercing to int64 must fail (INCORRECT_TYPE).
    with pytest.raises(Exception):
        sj.as_int64(sj.at_key(doc.root(), "name"))


def test_index_out_of_bounds_raises(doc):
    nums = sj.as_array(sj.at_key(doc.root(), "nums"))
    with pytest.raises(Exception):
        sj.array_at(nums, 99)


def test_malformed_parse():
    d = sj.Doc("{ not valid json ")
    assert not d.ok()
    assert d.error().value == EXPECTED["err_parse"]
    assert d.error() != sj.error_code.SUCCESS


def test_success_code():
    assert sj.error_code.SUCCESS.value == EXPECTED["success_code"]
