// Native C++ ground-truth oracle for the yaml-cpp binding (Layer-1 differential). Parses the
// SAME fixed YAML document the Python test drives through the binding and emits its observable
// properties (node Type() enums as ints, scalar coercions, container sizes, keyed/indexed
// access, the Dump() round-trip text, the malformed-input error, the bad-conversion error +
// fallback) as ONE JSON object on stdout. Shared compiler + shared yaml-cpp lib => any
// divergence is the binding layer's.
#include "yaml-cpp/yaml.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

static std::string jesc(const std::string& v) {
    std::string e = "\"";
    for (char c : v) {
        if (c == '\\' || c == '"') e += '\\';
        if (c == '\n') { e += "\\n"; continue; }
        e += c;
    }
    return e + "\"";
}

int main() {
    const std::string doc =
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
        "  y: 2\n";

    std::vector<std::pair<std::string, std::string>> kv;
    auto s = [&](const char* k, const std::string& v) { kv.emplace_back(k, jesc(v)); };
    auto i = [&](const char* k, long long v) { kv.emplace_back(k, std::to_string(v)); };
    auto b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto d = [&](const char* k, double v) { std::ostringstream o; o << v; kv.emplace_back(k, o.str()); };

    YAML::Node root = YAML::Load(doc);

    i("root_type", static_cast<long long>(root.Type()));   // NodeType::Map == 4
    i("root_size", static_cast<long long>(root.size()));
    b("root_is_map", root.IsMap());
    b("root_defined", root.IsDefined());

    YAML::Node name = root["name"];
    i("name_type", static_cast<long long>(name.Type()));   // Scalar == 2
    b("name_is_scalar", name.IsScalar());
    s("name_scalar", name.Scalar());
    s("name_as_str", name.as<std::string>());

    i("age_int", name ? root["age"].as<int>() : 0);
    d("ratio_dbl", root["ratio"].as<double>());
    b("flag_bool", root["flag"].as<bool>());

    YAML::Node items = root["items"];
    i("items_type", static_cast<long long>(items.Type()));  // Sequence == 3
    i("items_size", static_cast<long long>(items.size()));
    s("items_0", items[0].as<std::string>());
    s("items_2", items[2].as<std::string>());

    YAML::Node nested = root["nested"];
    i("nested_type", static_cast<long long>(nested.Type()));
    i("nested_x", nested["x"].as<int>());
    i("nested_y", nested["y"].as<int>());

    // null / missing key
    YAML::Node missing = root["does_not_exist"];
    b("missing_defined", missing.IsDefined());

    // enum ground-truth (NodeType::value)
    i("nt_undefined", static_cast<long long>(YAML::NodeType::Undefined));
    i("nt_null", static_cast<long long>(YAML::NodeType::Null));
    i("nt_scalar", static_cast<long long>(YAML::NodeType::Scalar));
    i("nt_sequence", static_cast<long long>(YAML::NodeType::Sequence));
    i("nt_map", static_cast<long long>(YAML::NodeType::Map));

    // enum ground-truth (EmitterStyle::value -- the BINDER-0022 collision twin,
    // bound parent-qualified as "EmitterStyle_value")
    i("es_default", static_cast<long long>(YAML::EmitterStyle::Default));
    i("es_block", static_cast<long long>(YAML::EmitterStyle::Block));
    i("es_flow", static_cast<long long>(YAML::EmitterStyle::Flow));

    // Dump round-trip text (byte-for-byte)
    s("dump", YAML::Dump(root));

    // Clone equality (Clone(root) parses+dumps to the same text)
    s("clone_dump", YAML::Dump(YAML::Clone(root)));

    // fallback coercion (BadConversion swallowed -> fallback)
    YAML::Node ht = YAML::Load("k: hello");
    i("fallback_int", ht["k"].as<int>(99));
    s("fallback_str", ht["missingkey"].as<std::string>(std::string("DEF")));

    // malformed-input error path
    {
        std::string kind = "none", what;
        try { YAML::Load("{ unclosed: [1, 2, 3"); }
        catch (const YAML::ParserException& e) { kind = "ParserException"; what = e.what(); }
        catch (const YAML::Exception& e) { kind = "Exception"; what = e.what(); }
        kv.emplace_back("parse_err_kind", jesc(kind));
        kv.emplace_back("parse_err_what", jesc(what));
    }
    // bad-conversion error path (as<int> on a string scalar)
    {
        std::string kind = "none", what;
        try { (void)YAML::Load("k: hello")["k"].as<int>(); }
        catch (const YAML::BadConversion& e) { kind = "BadConversion"; what = e.what(); }
        catch (const YAML::Exception& e) { kind = "Exception"; what = e.what(); }
        kv.emplace_back("badconv_kind", jesc(kind));
        kv.emplace_back("badconv_what", jesc(what));
    }

    std::cout << "{";
    for (size_t n = 0; n < kv.size(); ++n) {
        if (n) std::cout << ",";
        std::cout << "\"" << kv[n].first << "\":" << kv[n].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
