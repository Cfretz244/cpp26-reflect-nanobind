// Native C++ ground-truth oracle for the nlohmann::json binding (Layer-1
// differential). Exercises the SAME operations the Python test drives through the
// binding -- parse, dump, is_*(), size(), operator[], operator==, get<T> -- using
// nlohmann::json directly, and emits the results as a JSON object on stdout.
// run_gates captures that as expected.json; test_bindings.py asserts the bound
// module reproduces every value. Since the native harness and the bindings share
// one compiler and one json, any divergence is attributable to the binding layer.
//
// Uses plain nlohmann/json (no reflection, no skip annotations, no char_traits
// shim) -- this is the unmodified library's own behavior.
#include <nlohmann/json.hpp>
#include <cstdint>
#include <iostream>

using nlohmann::json;

int main() {
    json out;

    const char* obj_src = R"({"a":1,"b":[2,3],"c":"x"})";
    json j = json::parse(obj_src);
    out["obj_dump"]      = j.dump();
    out["obj_is_object"] = j.is_object();
    out["obj_size"]      = static_cast<std::int64_t>(j.size());
    out["b_dump"]        = j["b"].dump();
    out["b_is_array"]    = j["b"].is_array();
    out["b_size"]        = static_cast<std::int64_t>(j["b"].size());
    out["a_is_number"]   = j["a"].is_number();
    out["c_is_string"]   = j["c"].is_string();

    // scalar / array factories (mirror jsontest)
    out["int0_dump"]     = json(static_cast<std::int64_t>(0)).dump();
    out["text_dump"]     = json(std::string("hi")).dump();
    out["bool_dump"]     = json(true).dump();
    out["null_dump"]     = json(nullptr).dump();
    out["arr123_dump"]   = json::array({1, 2, 3}).dump();

    // equality
    out["eq_7_7"] = (json(static_cast<std::int64_t>(7)) == json(static_cast<std::int64_t>(7)));
    out["eq_7_8"] = (json(static_cast<std::int64_t>(7)) == json(static_cast<std::int64_t>(8)));

    // value extraction (json's get<T>)
    out["to_int_42"]      = json(static_cast<std::int64_t>(42)).get<std::int64_t>();
    out["to_text_hello"]  = json(std::string("hello")).get<std::string>();
    out["to_double_2_5"]  = json(2.5).get<double>();

    std::cout << out.dump() << std::endl;
    return 0;
}
