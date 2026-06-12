// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The exclude_ set holds the pImpl structs (BINDER-0019's home), stream
// types, iterator/walker internals, and the xpath surface.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^pugi::xml_node, \
    ^^pugi::xml_attribute, \
    ^^pugi::xml_document, \
    ^^pugi::xml_parse_result, \
    ^^pugi::xml_node_type, \
    ^^pugi::xml_encoding, \
    ^^pugi::xml_parse_status, \
    ^^pugitest, \
    ^^mirrorbind::exclude_<^^pugi::xml_writer, ^^std::basic_ostream, \
    ^^std::basic_istream, \
    ^^pugi::xml_node_struct, \
    ^^pugi::xml_attribute_struct, \
    ^^pugi::xml_node_iterator, \
    ^^pugi::xml_attribute_iterator, \
    ^^pugi::xml_named_node_iterator, \
    ^^pugi::xml_object_range, \
    ^^pugi::xml_text, \
    ^^pugi::xml_tree_walker, \
    ^^pugi::xpath_node, \
    ^^pugi::xpath_node_set, \
    ^^pugi::xpath_query, \
    ^^pugi::xpath_variable_set>
