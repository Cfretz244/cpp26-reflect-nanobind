"""Differential test for the pugixml binding (Layer-1 + Layer-3).

The binder binds pugixml's DOM handle classes head-on -- xml_document (parsed via
load_string, inheriting xml_node), xml_node (traversal/query), xml_attribute (typed
accessors), xml_parse_result (parse status) -- plus the node-type/encoding/parse-status
enums. oracle_native.cpp parses the SAME fixed XML document and drives the SAME
traversal/query/serialization sequence natively, emitting every observable as JSON; the
assertions compare the bound module against that ground truth (shared compiler + shared
pugixml source).

Default-argument VALUES are a C++26 reflection gap (P3096 exposes has_default_argument but
not the value), so every defaulted parameter is passed explicitly here -- the same value the
native oracle's C++ defaults use, so the two sides drive identical calls:
  options to load_string  -> parse_default = parse_cdata|parse_escapes|parse_wconv_attribute|parse_eol
  def to the as_* getters -> the same C++ defaults (0 / 0.0 / False)
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("pugixml_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# pugixml flag constants (free `const unsigned int`s are not bound; their numeric values
# are stable parts of the public API).
PARSE_DEFAULT = 0x04 | 0x10 | 0x40 | 0x20   # parse_cdata|parse_escapes|parse_wconv_attribute|parse_eol
FORMAT_RAW = 0x04 | 0x08                    # format_raw | format_no_declaration

DOC = (
    '<?xml version="1.0"?>'
    '<library name="central" open="true">'
    '<book id="1" pages="320" price="9.99">'
    '<title>The First</title>'
    '<author>Ada</author>'
    '</book>'
    '<book id="2" pages="512" price="19.50">'
    '<title>The Second</title>'
    '<author>Babbage</author>'
    '</book>'
    '<book id="3" pages="77" price="3.25">'
    '<title>The Third</title>'
    '</book>'
    '</library>'
)


@pytest.fixture(scope="module")
def lib():
    doc = m.xml_document()
    res = doc.load_string(DOC, PARSE_DEFAULT)
    assert bool(res) is True
    # keep doc alive (lib nodes reference it); return both
    el = doc.document_element()
    return doc, el


# --- Layer 1: differential vs the native oracle ---

def test_parse_result_differential():
    doc = m.xml_document()
    res = doc.load_string(DOC, PARSE_DEFAULT)
    assert bool(res) is (E["parse_ok"] is True)
    assert bool(res) is True
    assert res.status.value == E["parse_status"]
    assert res.status.value == E["parse_status_ok_value"]
    assert res.description() == E["parse_desc"]
    assert res.offset >= 0


def test_node_types_differential(lib):
    doc, el = lib
    assert el.name() == E["root_name"]
    assert el.type().value == E["root_type"]
    assert el.type().value == E["node_element_value"]
    assert doc.type().value == E["doc_type"]
    assert doc.type().value == E["node_document_value"]


def test_root_attributes_differential(lib):
    _, el = lib
    assert el.attribute("name").value() == E["lib_attr_name"]
    assert el.attribute("open").as_bool(False) is bool(E["lib_open_as_bool"])
    # attribute order
    order = []
    a = el.first_attribute()
    while not a.empty():
        order.append(a.name())
        a = a.next_attribute()
    assert ",".join(order) == E["lib_attr_order"]


def test_child_traversal_order_differential(lib):
    _, el = lib
    ids = []
    b = el.first_child()
    while not b.empty():
        ids.append(b.attribute("id").value())
        b = b.next_sibling()
    assert ",".join(ids) == E["book_id_order"]
    assert len(ids) == E["book_count"]


def test_typed_attribute_conversions_differential(lib):
    _, el = lib
    b1 = el.child("book")
    assert b1.attribute("id").as_int(0) == E["b1_id_int"]
    assert b1.attribute("pages").as_uint(0) == E["b1_pages_int"]
    # price as_double: compare via the same to_string the oracle used
    price = b1.attribute("price").as_double(0.0)
    assert ("%.6f" % price) == E["b1_price_str"]
    # missing attribute returns the supplied default
    assert b1.attribute("nope").as_int(-7) == E["b1_missing_default"]


def test_text_content_differential(lib):
    _, el = lib
    b1 = el.child("book")
    assert b1.child("title").child_value() == E["b1_title"]
    assert b1.child("author").child_value() == E["b1_author"]


def test_sparse_last_book_differential(lib):
    _, el = lib
    b3 = el.last_child()
    assert b3.attribute("id").value() == E["b3_id"]
    assert b3.child("author").empty() is bool(E["b3_author_empty"])
    assert b3.child("title").child_value() == E["b3_title"]


def test_sibling_navigation_differential(lib):
    _, el = lib
    b1 = el.child("book")
    b3 = el.last_child()
    assert b1.next_sibling().attribute("id").value() == E["b1_next_id"]
    assert b3.previous_sibling().attribute("id").value() == E["b3_prev_id"]
    assert (b1.parent() == el) is bool(E["b1_parent_is_lib"])


def test_handle_equality_differential(lib):
    _, el = lib
    assert (el.first_child() == el.child("book")) is bool(E["first_child_eq_child"])
    # doc != lib uses xml_node::operator!= (xml_document is-a xml_node)
    doc, _el = lib
    assert (doc != el) is bool(E["doc_neq_lib"])


def test_serialization_differential(lib):
    _, el = lib
    b1 = el.child("book")
    assert m.to_xml_raw(b1) == E["b1_raw"]
    assert m.to_xml_raw(el) == E["full_raw"]


def test_parse_failure_differential():
    bad = m.xml_document()
    bres = bad.load_string("<a><b></a>", PARSE_DEFAULT)
    assert bool(bres) is bool(E["bad_ok"])
    assert bool(bres) is False
    assert bres.status.value == E["bad_status"]
    assert bres.status.value == E["status_end_mismatch_value"]


# --- Layer 3: structural invariants ---

def test_inheritance_is_real_base():
    # xml_document is in the bind set, so xml_node is its REAL Python base, not flattened.
    assert issubclass(m.xml_document, m.xml_node)
    doc = m.xml_document()
    assert isinstance(doc, m.xml_node)


def test_surface_present():
    for meth in ("type", "name", "value", "child", "attribute", "child_value",
                 "first_child", "last_child", "next_sibling", "previous_sibling",
                 "parent", "empty", "root"):
        assert hasattr(m.xml_node, meth), meth
    for meth in ("name", "value", "as_int", "as_uint", "as_double", "as_bool",
                 "next_attribute", "empty"):
        assert hasattr(m.xml_attribute, meth), meth
    for meth in ("load_string", "document_element"):
        assert hasattr(m.xml_document, meth), meth
    for attr in ("status", "offset", "encoding", "description"):
        assert hasattr(m.xml_parse_result, attr), attr


def test_parse_result_bool_dunder():
    # operator bool() -> __bool__: a failed-state default xml_parse_result is falsy.
    doc = m.xml_document()
    res = doc.load_string(DOC, PARSE_DEFAULT)
    assert bool(res) is True
    bad = m.xml_document()
    bres = bad.load_string("<a><b></a>", PARSE_DEFAULT)
    assert bool(bres) is False


def test_enum_completeness():
    # all xml_node_type values present and ordered as declared
    nt = m.xml_node_type
    assert nt.node_null.value == 0
    assert nt.node_document.value == 1
    assert nt.node_element.value == 2
    assert nt.node_pcdata.value == 3
    # xml_parse_status carries status_ok == 0
    assert m.xml_parse_status.status_ok.value == 0
