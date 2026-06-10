// Native C++ ground-truth oracle for the toml++ binding (Layer-1 differential). Parses the
// EXACT same TOML document the Python test drives through the bound module, via the SAME
// library (header-only, TOML_EXCEPTIONS=0, TOML_IMPLEMENTATION) and the SAME tomlfix walkers,
// and emits every observable as one JSON object on stdout. Shared compiler + shared library
// => any divergence is the binding layer's. The Python side calls tomlfix through the bound
// module and the real bound date/time/node_type/parse_error types.
#include "../binding/tomlfix.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

static const std::string DOC = R"(title = "TOML Example"
enabled = true
pi = 3.14159
count = 42
neg = -7
big = 9223372036854775807
name = "café"
ratio = 1.5e3
the_date = 2024-03-16
the_time = 13:45:06
local_dt = 1979-05-27T07:32:00
offset_dt = 1979-05-27T07:32:00-07:00
ints = [1, 2, 3, 5, 8]
mixed = [10, "two", true]
[server]
host = "localhost"
port = 8080
[server.limits]
max = 1000
)";

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto esc = [](const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') e += "\\n";
            else e += c;
        }
        return e + "\"";
    };
    auto add_s = [&](const char* k, const std::string& v) { kv.emplace_back(k, esc(v)); };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto add_d = [&](const char* k, double v) {
        std::ostringstream ss; ss.precision(17); ss << v; kv.emplace_back(k, ss.str());
    };
    auto add_arr_i = [&](const char* k, const std::vector<std::int64_t>& v) {
        std::string s = "[";
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) s += ","; s += std::to_string(v[i]); }
        kv.emplace_back(k, s + "]");
    };
    auto add_arr_s = [&](const char* k, const std::vector<std::string>& v) {
        std::string s = "[";
        for (std::size_t i = 0; i < v.size(); ++i) { if (i) s += ","; s += esc(v[i]); }
        kv.emplace_back(k, s + "]");
    };
    auto nt_name = [](toml::node_type t) -> std::string {
        switch (t) {
            case toml::node_type::none: return "none";
            case toml::node_type::table: return "table";
            case toml::node_type::array: return "array";
            case toml::node_type::string: return "string";
            case toml::node_type::integer: return "integer";
            case toml::node_type::floating_point: return "floating_point";
            case toml::node_type::boolean: return "boolean";
            case toml::node_type::date: return "date";
            case toml::node_type::time: return "time";
            case toml::node_type::date_time: return "date_time";
        }
        return "?";
    };

    // --- parse success + shape ---
    add_b("parse_ok", tomlfix::parse_ok(DOC));
    add_i("table_size", tomlfix::table_size(DOC));
    add_arr_s("top_keys", tomlfix::top_keys(DOC));

    // --- node_type of representative keys (the real enum, by value) ---
    auto ti = [&](const char* key) { return static_cast<std::int64_t>(*tomlfix::type_of(DOC, key)); };
    add_i("ntype_count", ti("count"));
    add_i("ntype_pi", ti("pi"));
    add_i("ntype_enabled", ti("enabled"));
    add_i("ntype_name", ti("name"));
    add_i("ntype_the_date", ti("the_date"));
    add_i("ntype_the_time", ti("the_time"));
    add_i("ntype_local_dt", ti("local_dt"));
    add_i("ntype_ints", ti("ints"));
    add_i("ntype_server", ti("server"));
    add_s("ntype_count_name", nt_name(*tomlfix::type_of(DOC, "count")));
    add_s("ntype_server_name", nt_name(*tomlfix::type_of(DOC, "server")));

    // --- coerced scalars (the library's value<T>()) ---
    add_i("count", *tomlfix::get_int(DOC, "count"));
    add_i("neg", *tomlfix::get_int(DOC, "neg"));
    add_i("big", *tomlfix::get_int(DOC, "big"));
    add_d("pi", *tomlfix::get_float(DOC, "pi"));
    add_d("ratio", *tomlfix::get_float(DOC, "ratio"));
    add_b("enabled", *tomlfix::get_bool(DOC, "enabled"));
    add_s("name", *tomlfix::get_string(DOC, "name"));
    add_i("nested_port", *tomlfix::get_int(DOC, "server.port"));
    add_i("deep_max", *tomlfix::get_int(DOC, "server.limits.max"));
    add_s("nested_host", *tomlfix::get_string(DOC, "server.host"));
    // permissive coercion: get_int on a float key yields nullopt (no value)
    add_b("int_of_pi_empty", !tomlfix::get_int(DOC, "pi").has_value());

    // --- date / time / date_time (the real bound value structs) ---
    auto d = *tomlfix::get_date(DOC, "the_date");
    add_i("date_year", d.year); add_i("date_month", d.month); add_i("date_day", d.day);
    auto tm = *tomlfix::get_time(DOC, "the_time");
    add_i("time_hour", tm.hour); add_i("time_min", tm.minute); add_i("time_sec", tm.second);
    auto ldt = *tomlfix::get_datetime(DOC, "local_dt");
    add_i("ldt_year", ldt.date.year); add_i("ldt_hour", ldt.time.hour);
    add_b("ldt_is_local", ldt.is_local());
    auto odt = *tomlfix::get_datetime(DOC, "offset_dt");
    add_b("odt_is_local", odt.is_local());
    add_i("odt_offset_min", odt.offset.has_value() ? odt.offset->minutes : 0);

    // --- arrays ---
    add_i("ints_size", tomlfix::array_size(DOC, "ints"));
    add_arr_i("ints_vals", tomlfix::int_array(DOC, "ints"));
    add_i("mixed_size", tomlfix::array_size(DOC, "mixed"));
    {
        std::vector<std::string> names;
        for (auto t : tomlfix::array_types(DOC, "mixed")) names.push_back(nt_name(t));
        add_arr_s("mixed_types", names);
    }

    // --- round-trip serialization (the library's formatters) ---
    add_s("toml_roundtrip", tomlfix::to_toml(DOC));
    add_s("json_roundtrip", tomlfix::to_json(DOC));

    // --- error path (real parse_error: description + line/column) ---
    {
        const std::string bad = "x = = 3\n";
        add_b("bad_parse_ok", tomlfix::parse_ok(bad));
        auto err = tomlfix::parse_error_of(bad);
        add_s("err_desc", std::string(err->description()));
        add_i("err_line", static_cast<std::int64_t>(err->source().begin.line));
        add_i("err_col", static_cast<std::int64_t>(err->source().begin.column));
    }

    std::cout << "{";
    for (std::size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
