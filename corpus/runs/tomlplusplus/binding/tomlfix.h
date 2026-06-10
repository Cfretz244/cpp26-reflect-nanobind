// Fixture header for the toml++ run.
//
// WHY a fixture is needed here (not to re-implement library behavior, but because the real
// API cannot reach Python under the current binder):
//
//  1. The toml++ document tree -- toml::node and its leaves table/array/value<T> -- CANNOT
//     be bound head-on: every one of those classes declares
//         virtual bool is_homogeneous(node_type, node*& first_nonmatch) [const] noexcept;
//     and the binder hard-errors on the non-const lvalue-reference-to-pointer (node*&)
//     out-parameter instead of skipping it (findings_draft/
//     binder-lvalue-ref-to-pointer-param.md; per-overload nb::exclude_ does not take here).
//     So tree NAVIGATION must happen in C++ and surface plain Python values / the bound
//     value-structs. All parsing, coercion, and serialization below is the LIBRARY's own
//     code -- the fixture only walks and hands back results.
//  2. toml::parse(...) is a large overload set (string_view / u8string_view / std::istream& /
//     wstring_view source paths); a single std::string entry point is provided.
//  3. node::value<T>() is a consteval template accessor; the coercion is the library's, the
//     wrappers just pick T.
//  4. Serialization runs through std::ostream (no Python representation); the wrappers run
//     the library's real toml_formatter / json_formatter into a std::string.
//
// The date/time VALUE structs (toml::date/time/date_time/time_offset), the toml::node_type
// enum, and toml::parse_error/source_region/source_position ARE bound head-on (see
// binding.cpp); the fixture returns those real library types directly, so the differential
// checks real bound objects, not fixture stand-ins.
//
// TOML_EXCEPTIONS=0 + TOML_IMPLEMENTATION must precede every toml++ include in every TU.
#pragma once

#define TOML_EXCEPTIONS 0
#define TOML_IMPLEMENTATION
#include <toml++/toml.hpp>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace tomlfix {
namespace detail {

// ---- internal: resolve a dotted key path ("a.b.c") to a node, navigating the tree ----
// In a nested namespace so reflect_<^^tomlfix> does not surface it (it takes/returns the
// unbound toml::table&/toml::node*, which have no Python representation).
inline const toml::node* resolve(const toml::table& root, const std::string& dotted) {
    const toml::node* cur = &root;
    std::size_t start = 0;
    while (start <= dotted.size()) {
        std::size_t dot = dotted.find('.', start);
        std::string part = dotted.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        const toml::table* t = cur->as_table();
        if (!t) return nullptr;
        cur = t->get(part);
        if (!cur) return nullptr;
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return cur;
}

} // namespace detail

// ---- parse entry point (real parser) + success / error introspection ----
inline bool parse_ok(const std::string& doc) { return toml::parse(doc).succeeded(); }

// The real toml::parse_error (a bound type). Returned by value via optional; nullopt on success.
inline std::optional<toml::parse_error> parse_error_of(const std::string& doc) {
    auto r = toml::parse(doc);
    if (r.succeeded()) return std::nullopt;
    return r.error();
}

// ---- node_type of a key (the real discriminant enum, bound head-on) ----
inline std::optional<toml::node_type> type_of(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc);
    if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key);
    if (!n) return std::nullopt;
    return n->type();
}

// ---- coercing scalar accessors (the library's node::value<T>() retrieval) ----
inline std::optional<std::int64_t> get_int(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<std::int64_t>();
}
inline std::optional<double> get_float(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<double>();
}
inline std::optional<bool> get_bool(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<bool>();
}
inline std::optional<std::string> get_string(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<std::string>();
}

// ---- date/time accessors: return the REAL bound value structs ----
inline std::optional<toml::date> get_date(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<toml::date>();
}
inline std::optional<toml::time> get_time(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<toml::time>();
}
inline std::optional<toml::date_time> get_datetime(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::nullopt;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return std::nullopt;
    return n->value<toml::date_time>();
}

// ---- container shape ----
inline std::int64_t table_size(const std::string& doc) {
    auto r = toml::parse(doc); if (!r.succeeded()) return -1;
    return static_cast<std::int64_t>(r.table().size());
}
inline std::int64_t array_size(const std::string& doc, const std::string& key) {
    auto r = toml::parse(doc); if (!r.succeeded()) return -1;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return -1;
    const toml::array* a = n->as_array(); if (!a) return -1;
    return static_cast<std::int64_t>(a->size());
}
inline std::vector<std::string> top_keys(const std::string& doc) {
    std::vector<std::string> out;
    auto r = toml::parse(doc); if (!r.succeeded()) return out;
    for (auto&& [k, v] : r.table()) out.emplace_back(std::string(k.str()));
    return out;
}
// element node_type sequence of an array (real enum values)
inline std::vector<toml::node_type> array_types(const std::string& doc, const std::string& key) {
    std::vector<toml::node_type> out;
    auto r = toml::parse(doc); if (!r.succeeded()) return out;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return out;
    const toml::array* a = n->as_array(); if (!a) return out;
    for (std::size_t i = 0; i < a->size(); ++i)
        if (const toml::node* e = a->get(i)) out.push_back(e->type());
    return out;
}
// int elements of an integer array (coerced via the library)
inline std::vector<std::int64_t> int_array(const std::string& doc, const std::string& key) {
    std::vector<std::int64_t> out;
    auto r = toml::parse(doc); if (!r.succeeded()) return out;
    const toml::node* n = detail::resolve(r.table(), key); if (!n) return out;
    const toml::array* a = n->as_array(); if (!a) return out;
    for (std::size_t i = 0; i < a->size(); ++i)
        if (const toml::node* e = a->get(i))
            if (auto v = e->value<std::int64_t>()) out.push_back(*v);
    return out;
}

// ---- serializers (the library's real formatters) ----
inline std::string to_toml(const std::string& doc) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::string();
    std::ostringstream ss; ss << toml::toml_formatter{ r.table() }; return ss.str();
}
inline std::string to_json(const std::string& doc) {
    auto r = toml::parse(doc); if (!r.succeeded()) return std::string();
    std::ostringstream ss; ss << toml::json_formatter{ r.table() }; return ss.str();
}

} // namespace tomlfix
