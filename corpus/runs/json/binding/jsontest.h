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
#include <string>
#include <cstdint>

// [test fixture] std::char_traits<unsigned char>.
//
// nlohmann/json's binary-format machinery (detail::output_adapter<std::uint8_t>,
// output_string_adapter) NAMES std::basic_string<unsigned char> but never
// instantiates it in normal use -- which is fortunate, because std::char_traits
// is only specialized for char/wchar_t/char8_t/char16_t/char32_t, so the byte
// string is ill-formed to instantiate (a latent json bug). Our reflection binder
// *does* instantiate it when it binds that machinery, so we supply the missing
// char_traits here -- defined byte-for-byte like libc++'s own char_traits<char8_t>
// (char8_t is the same width/representation). This is a TEST-SIDE declaration so
// the binder can be validated against the real, unmodified nlohmann::json; json
// itself is left pristine. Declared before <nlohmann/json.hpp> so the
// specialization is visible at every instantiation point.
namespace std {
template <>
struct char_traits<unsigned char>
    : __char_traits_base<unsigned char, unsigned int, static_cast<unsigned int>(EOF)> {
    static constexpr int compare(const char_type* a, const char_type* b, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i) {
            if (lt(a[i], b[i])) return -1;
            if (lt(b[i], a[i])) return 1;
        }
        return 0;
    }
    static constexpr size_t length(const char_type* s) noexcept {
        size_t i = 0;
        while (!eq(s[i], char_type())) ++i;
        return i;
    }
    static constexpr const char_type* find(const char_type* s, size_t n,
                                           const char_type& a) noexcept {
        for (size_t i = 0; i < n; ++i)
            if (eq(s[i], a)) return s + i;
        return nullptr;
    }
};
}  // namespace std

// Make the [[=reflect::skip]] annotation visible before json is parsed, so the
// json test copy can mark its binary-format internals (detail::output_adapter
// family, byte_container_with_subtype) as not-to-be-bound. These types are
// implementation details that the binder would otherwise transitively bind (they
// appear in basic_json::to_cbor/to_msgpack/... signatures and the binary_t base);
// binding them drags in un-castable byte types. Skipping them via the binder's own
// supported annotation keeps the public json surface bindable. See the
// `[[=::nanobind::reflect::skip{}]]` markers added in json.hpp.
#include <nanobind/nb_reflect_annotations.h>

// Enable the opt-in skip annotations inside json (see NLOHMANN_JSON_NB_SKIP in
// json.hpp). Only the binding TUs do this; a plain `#include <nlohmann/json.hpp>`
// (e.g. the Gate-1 probe) leaves json standalone and unannotated. Guarded on
// P3394 support: the emit lane's GENERATED TU re-includes this header under
// the production compiler, where the annotations (already consumed at
// generation time) must compile away.
#if defined(__has_feature)
#  if __has_feature(annotation_attributes)
#    define NLOHMANN_JSON_NB_SKIP [[=::nanobind::reflect::skip{}]]
#  endif
#endif
#ifndef NLOHMANN_JSON_NB_SKIP
#  define NLOHMANN_JSON_NB_SKIP
#endif

#include <nlohmann/json.hpp>

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
