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
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(pugixml_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
