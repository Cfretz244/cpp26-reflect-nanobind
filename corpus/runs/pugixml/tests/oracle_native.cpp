// Native C++ ground-truth oracle for the pugixml binding (Layer-1 differential). Parses the
// EXACT XML document the Python test parses through the bound module, then drives the SAME
// traversal/query/serialization sequence and emits every observable as JSON. Shared compiler
// + shared pugixml source => any divergence is the binding layer's.
//
// pugixml.cpp is #include'd directly: it is a single self-contained translation unit, and the
// native-oracle build helper compiles only this one .cpp (it does not pull in extra_sources the
// way the module build does). This gives the oracle the real library symbols.
#include "../../../libs/pugixml/src/pugixml.cpp"

#include "../binding/pugitest.h"

#include "pugixml.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// The fixed document parsed identically on both sides (mirrored verbatim in test_bindings.py).
static const char* kDoc =
    "<?xml version=\"1.0\"?>"
    "<library name=\"central\" open=\"true\">"
    "<book id=\"1\" pages=\"320\" price=\"9.99\">"
    "<title>The First</title>"
    "<author>Ada</author>"
    "</book>"
    "<book id=\"2\" pages=\"512\" price=\"19.50\">"
    "<title>The Second</title>"
    "<author>Babbage</author>"
    "</book>"
    "<book id=\"3\" pages=\"77\" price=\"3.25\">"
    "<title>The Third</title>"
    "</book>"
    "</library>";

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') { e += "\\n"; }
            else if (c == '\t') { e += "\\t"; }
            else { e += c; }
        }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_string(kDoc);

    // --- parse result ---
    add_b("parse_ok", static_cast<bool>(res));
    add_i("parse_status", static_cast<std::int64_t>(res.status));
    add_s("parse_desc", res.description());
    add_i("parse_status_ok_value", static_cast<std::int64_t>(pugi::status_ok));

    // --- document element + node types ---
    pugi::xml_node lib = doc.document_element();
    add_s("root_name", lib.name());
    add_i("root_type", static_cast<std::int64_t>(lib.type()));
    add_i("doc_type", static_cast<std::int64_t>(doc.type()));
    add_i("node_element_value", static_cast<std::int64_t>(pugi::node_element));
    add_i("node_document_value", static_cast<std::int64_t>(pugi::node_document));

    // --- attributes on the root (order + typed conversions) ---
    add_s("lib_attr_name", lib.attribute("name").value());
    add_b("lib_open_as_bool", lib.attribute("open").as_bool());
    {
        std::string order;
        for (pugi::xml_attribute a = lib.first_attribute(); a; a = a.next_attribute()) {
            if (!order.empty()) order += ",";
            order += a.name();
        }
        add_s("lib_attr_order", order);
    }

    // --- child traversal order (book ids in document order) ---
    {
        std::string ids;
        int count = 0;
        for (pugi::xml_node b = lib.first_child(); b; b = b.next_sibling()) {
            if (!ids.empty()) ids += ",";
            ids += b.attribute("id").value();
            ++count;
        }
        add_s("book_id_order", ids);
        add_i("book_count", count);
    }

    // --- typed attribute conversions on the first book ---
    pugi::xml_node b1 = lib.child("book");
    add_i("b1_id_int", b1.attribute("id").as_int());
    add_i("b1_pages_int", b1.attribute("pages").as_uint());
    {
        // Emit the parsed double as a value, not via std::to_string: the latter's
        // double formatting diverges across standard libraries (libc++ renders the
        // classic 6-decimal "%f" form "9.990000"; libstdc++ in C++26 mode renders the
        // shortest round-trippable form "9.99"), which would make the differential
        // string compare a library artifact rather than a binding observation. The
        // test compares numerically (pytest.approx) against this value.
        double price = b1.attribute("price").as_double();
        kv.emplace_back("b1_price", std::to_string(price));
    }
    add_i("b1_missing_default", b1.attribute("nope").as_int(-7));

    // --- text content via child_value ---
    add_s("b1_title", b1.child("title").child_value());
    add_s("b1_author", b1.child("author").child_value());

    // --- last book has no author (next_sibling / previous_sibling + empty) ---
    pugi::xml_node b3 = lib.last_child();
    add_s("b3_id", b3.attribute("id").value());
    add_b("b3_author_empty", b3.child("author").empty());
    add_s("b3_title", b3.child("title").child_value());

    // --- sibling navigation ---
    add_s("b1_next_id", b1.next_sibling().attribute("id").value());
    add_s("b3_prev_id", b3.previous_sibling().attribute("id").value());
    add_b("b1_parent_is_lib", b1.parent() == lib);

    // --- comparison / identity (handle equality) ---
    add_b("first_child_eq_child", lib.first_child() == lib.child("book"));
    add_b("doc_neq_lib", doc != lib);

    // --- serialization (via the pugitest fixture; the library's own print path) ---
    add_s("b1_raw", pugitest::to_xml_raw(b1));
    add_s("full_raw", pugitest::to_xml_raw(lib));

    // --- a parse failure (status path) ---
    pugi::xml_document bad;
    pugi::xml_parse_result bres = bad.load_string("<a><b></a>");
    add_b("bad_ok", static_cast<bool>(bres));
    add_i("bad_status", static_cast<std::int64_t>(bres.status));
    add_i("status_end_mismatch_value", static_cast<std::int64_t>(pugi::status_end_element_mismatch));

    // --- xml_text: the typed text handle (reads, defaults, mutation + round-trip).
    // A separate small document so the mutation never perturbs the kDoc observables.
    {
        pugi::xml_document tdoc;
        pugi::xml_parse_result tres = tdoc.load_string("<n><v>42</v><s>hi</s></n>");
        add_b("text_parse_ok", static_cast<bool>(tres));
        pugi::xml_node n = tdoc.document_element();
        pugi::xml_text vt = n.child("v").text();
        add_b("text_v_empty", vt.empty());
        add_s("text_get", vt.get());
        add_i("text_as_int", vt.as_int());
        kv.emplace_back("text_as_double", std::to_string(vt.as_double()));
        add_i("text_s_as_int_default", n.child("s").text().as_int(-5));
        add_s("text_s_string", n.child("s").text().as_string());
        add_b("text_missing_empty", n.child("missing").text().empty());
        add_b("text_set_ok", n.child("s").text().set("bye"));
        add_b("text_set_int_ok", n.child("v").text().set(7));
        add_s("text_after_set_raw", pugitest::to_xml_raw(n));
    }

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
