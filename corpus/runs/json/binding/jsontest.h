// Shared definitions for the nlohmann/json run, included by BOTH the generator (gen.cpp) and
// the bindings module (binding.cpp), mirroring the codegen test pattern.
//
// WHY a helper namespace: nlohmann::json's entire value-ingestion/extraction layer is member
// TEMPLATES (the CompatibleType constructors, get<T>(), parse(), ...), which the binder correctly
// skips. So Python has no way to put a scalar into a json or read one out. These free functions
// are TEST FIXTURES: they use json's templated C++ API internally to produce/consume values, so a
// Python test can obtain populated json objects and then exercise the *binder-exposed* surface
// (dump(), is_*(), size(), operator[], ==). They do not themselves test the binder.
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

namespace jsontest {

using nlohmann::json;

// --- factories (produce populated json from plain scalars Python can pass) ---
inline json null_value()              { return json(nullptr); }
inline json boolean(bool b)           { return json(b); }
inline json integer(std::int64_t i)   { return json(i); }
inline json number(double d)          { return json(d); }
inline json text(const std::string& s){ return json(s); }
inline json parse(const std::string& s){ return json::parse(s); }

// fixed sample structures (ground truth shared with the native oracle)
inline json array_123()  { return json::array({1, 2, 3}); }
inline json object_ab()  { return json::parse(R"({"a":1,"b":[2,3],"c":"x"})"); }

// --- extractors (read a typed value back out, via the templated get<T> in C++) ---
inline std::int64_t to_int(const json& j)  { return j.get<std::int64_t>(); }
inline double       to_double(const json& j){ return j.get<double>(); }
inline std::string  to_text(const json& j) { return j.get<std::string>(); }

}  // namespace jsontest
