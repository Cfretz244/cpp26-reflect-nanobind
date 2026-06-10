// pugitest -- serialization fixture for the pugixml run, shared by binding.cpp and the
// native oracle.
//
// pugixml only serializes a node/document through an xml_writer or a std::ostream front
// (xml_node::print / xml_document::save) -- neither has a nanobind caster, and we do not
// want xml_writer / std::basic_ostream in the bind set. to_xml() drives the real
// xml_node::print into a std::ostringstream and returns the resulting std::string, so the
// library's own serialization is observable from Python as text. This is the ONLY thing the
// fixture provides; parsing and traversal are bound head-on off the real DOM classes.
#pragma once

#include "pugixml.hpp"

#include <sstream>
#include <string>

namespace pugitest {

// Serialize a node's subtree to a string using pugixml's own xml_node::print, with raw
// (no-indent) formatting and the XML declaration omitted so the output is stable and easy
// to compare. node.print writes into the ostringstream via the library's real writer path.
inline std::string to_xml_raw(const pugi::xml_node& node) {
    std::ostringstream oss;
    node.print(oss, "", pugi::format_raw | pugi::format_no_declaration);
    return oss.str();
}

// Serialize with the default (indented) formatting, declaration omitted.
inline std::string to_xml_indented(const pugi::xml_node& node) {
    std::ostringstream oss;
    node.print(oss, "  ", pugi::format_indent | pugi::format_no_declaration);
    return oss.str();
}

}  // namespace pugitest
