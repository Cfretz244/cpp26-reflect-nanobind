// Single-stage bindings for pugixml v1.15 (Tier 3: lightweight DOM handle objects +
// inheritance + enums). The DOM classes are bound head-on:
//   xml_node       -- the lightweight node handle (traversal/query/mutation)
//   xml_attribute  -- the attribute handle (name/value + typed accessors)
//   xml_document   -- the DOM root, publicly inheriting xml_node (real Python base)
//   xml_parse_result -- the parse-status struct (data members + description())
// plus the xml_node_type / xml_encoding / xml_parse_status enums.
//
// The pugitest fixture supplies ONLY serialization (to_xml_*), which pugixml exposes only
// through an xml_writer / std::ostream front Python cannot drive. The exclude_ pack (in
// meta.toml's reflect_args) makes the writer/stream/iterator/range/text/tree-walker/XPath
// types opaque so the methods mentioning them are skipped (BINDER-0014).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "pugitest.h"

namespace nb = nanobind;

NB_MODULE(pugixml_ext, m) {
    nb::reflect_<^^pugi::xml_node,
                 ^^pugi::xml_attribute,
                 ^^pugi::xml_document,
                 ^^pugi::xml_parse_result,
                 ^^pugi::xml_node_type,
                 ^^pugi::xml_encoding,
                 ^^pugi::xml_parse_status,
                 ^^pugitest,
                 ^^nb::exclude_<^^pugi::xml_writer, ^^std::basic_ostream,
                                ^^std::basic_istream,
                                ^^pugi::xml_node_struct,
                                ^^pugi::xml_attribute_struct,
                                ^^pugi::xml_node_iterator,
                                ^^pugi::xml_attribute_iterator,
                                ^^pugi::xml_named_node_iterator,
                                ^^pugi::xml_object_range,
                                ^^pugi::xml_text,
                                ^^pugi::xml_tree_walker,
                                ^^pugi::xpath_node,
                                ^^pugi::xpath_node_set,
                                ^^pugi::xpath_query,
                                ^^pugi::xpath_variable_set>>(m);
}
