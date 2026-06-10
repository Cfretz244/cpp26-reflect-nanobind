// Native C++ ground-truth oracle for the simdjson DOM binding (Layer-1 differential). Parses the
// EXACT JSON document the Python test parses through the bound module, drives the SAME navigation/
// coercion/discrimination sequence through simdjson's own DOM API, and emits every observable as
// one JSON object on stdout. Shared compiler + shared simdjson source => any divergence is the
// binding layer's.
//
// simdjson.cpp is #include'd directly: the amalgamation is one self-contained TU, and the native-
// oracle build helper compiles only this one .cpp (it does not pull in extra_sources the way the
// module build does). This gives the oracle the real library symbols. (pugixml precedent.)
#include "../../../libs/simdjson/singleheader/simdjson.cpp"

#include "simdjson.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace sd = simdjson;
namespace dom = simdjson::dom;

// The fixed document parsed identically on both sides (mirrored verbatim in test_bindings.py).
static const char* kDoc =
    "{"
    "\"name\": \"central\","
    "\"open\": true,"
    "\"closed\": false,"
    "\"count\": 3,"
    "\"big\": 9999999999,"
    "\"price\": 19.5,"
    "\"nothing\": null,"
    "\"tags\": [\"a\", \"b\", \"c\"],"
    "\"nums\": [10, 20, 30],"
    "\"meta\": {\"id\": 7, \"label\": \"x\"}"
    "}";

static std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
        else o.push_back(c);
    }
    return o;
}

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const std::string& k, const std::string& v) {
        kv.emplace_back(k, "\"" + esc(v) + "\"");
    };
    auto add_r = [&](const std::string& k, const std::string& raw) { kv.emplace_back(k, raw); };

    dom::parser parser;
    dom::element root = parser.parse(std::string(kDoc));

    // --- type discriminants on the root object ---
    add_r("root_type", std::to_string(static_cast<int>(root.type())));
    add_r("root_is_object", root.is_object() ? "true" : "false");
    add_r("root_size", std::to_string(dom::object(root).size()));

    // --- scalar reads ---
    add_s("name", std::string(std::string_view(root["name"])));
    add_r("open", bool(root["open"]) ? "true" : "false");
    add_r("closed", bool(root["closed"]) ? "true" : "false");
    add_r("count", std::to_string(int64_t(root["count"])));
    add_r("big", std::to_string(uint64_t(root["big"])));
    {
        double p = double(root["price"]);
        add_r("price", std::to_string(p));
    }
    add_r("nothing_is_null", root["nothing"].is_null() ? "true" : "false");

    // --- per-value type discriminants ---
    // operator[] yields a simdjson_result<element>; materialize the element before .type().
    auto type_of = [&](const char* k) {
        dom::element e = root[k];
        return static_cast<int>(e.type());
    };
    add_r("type_name", std::to_string(type_of("name")));
    add_r("type_open", std::to_string(type_of("open")));
    add_r("type_count", std::to_string(type_of("count")));
    add_r("type_big", std::to_string(type_of("big")));
    add_r("type_price", std::to_string(type_of("price")));
    add_r("type_nothing", std::to_string(type_of("nothing")));
    add_r("type_tags", std::to_string(type_of("tags")));
    add_r("type_meta", std::to_string(type_of("meta")));

    // --- is_number on various values ---
    add_r("count_is_number", root["count"].is_number() ? "true" : "false");
    add_r("price_is_number", root["price"].is_number() ? "true" : "false");
    add_r("name_is_number", root["name"].is_number() ? "true" : "false");
    add_r("count_is_int64", root["count"].is_int64() ? "true" : "false");
    add_r("price_is_double", root["price"].is_double() ? "true" : "false");

    // --- arrays ---
    {
        dom::array tags = root["tags"];
        add_r("tags_size", std::to_string(tags.size()));
        std::string joined;
        for (dom::element e : tags) {
            if (!joined.empty()) joined += ",";
            joined += std::string(std::string_view(e));
        }
        add_s("tags_joined", joined);
        add_s("tags_at1", std::string(std::string_view(tags.at(1).value())));
    }
    {
        dom::array nums = root["nums"];
        int64_t sum = 0;
        for (dom::element e : nums) { sum += int64_t(e); }
        add_r("nums_sum", std::to_string(sum));
        add_r("nums_at2", std::to_string(int64_t(nums.at(2).value())));
    }

    // --- nested object ---
    {
        dom::object meta = root["meta"];
        add_r("meta_size", std::to_string(meta.size()));
        add_r("meta_id", std::to_string(int64_t(meta["id"].value())));
        add_s("meta_label", std::string(std::string_view(meta["label"].value())));
        std::string keys;
        for (auto field : meta) {
            if (!keys.empty()) keys += ",";
            keys += std::string(field.key);
        }
        add_s("meta_keys", keys);
    }

    // --- JSON pointer navigation ---
    add_r("ptr_nums1", std::to_string(int64_t(root.at_pointer("/nums/1").value())));
    add_s("ptr_metalabel", std::string(std::string_view(root.at_pointer("/meta/label").value())));

    // --- top-level object keys in document order ---
    {
        dom::object obj = root;
        std::string keys;
        for (auto field : obj) {
            if (!keys.empty()) keys += ",";
            keys += std::string(field.key);
        }
        add_s("root_keys", keys);
    }

    // --- error paths (raw error_code as int) ---
    add_r("err_missing_key", std::to_string(static_cast<int>(root.at_key("nope").error())));
    add_r("err_wrong_type", std::to_string(static_cast<int>(root["name"].get_int64().error())));
    add_r("err_oob", std::to_string(static_cast<int>(dom::array(root["nums"]).at(99).error())));
    {
        dom::parser p2;
        auto bad = p2.parse(std::string("{ not valid json "));
        add_r("err_parse", std::to_string(static_cast<int>(bad.error())));
    }
    add_r("success_code", std::to_string(static_cast<int>(sd::SUCCESS)));

    // emit
    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
