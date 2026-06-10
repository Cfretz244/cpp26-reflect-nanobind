// Fixture namespace for the tl::expected run. Everything observed on the returned
// objects in the tests goes through the directly bound real API (has_value/__bool__/
// value/error/swap/ctors); this namespace supplies only what that API cannot express
// to Python:
//   - factories: expected<T,E>'s value/error converting constructors are all templates
//     (enable_if'd U&& / unexpected<G>), which the binder skips by design -- same
//     pattern as abseil_statusor's sotest.
//   - from_unexpected: drives the REAL expected(const unexpected<G>&) converting ctor
//     natively, with the bound tl::unexpected<std::string> crossing from Python.
//   - thin wrappers over all-template fronts: the free operator== family, the member
//     templates value_or<U>/map<F>/and_then<F> (F/U have no defaults, so the binder's
//     default-instantiation path cannot touch them).
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <string>
#include <string_view>

#include <tl/expected.hpp>

namespace extest {

// --- factories (converting ctors are templates; binder skips ctor templates) ---
inline tl::expected<int, std::string> ok_int(int v) { return v; }
inline tl::expected<int, std::string> err_int(std::string_view msg) {
    return tl::make_unexpected(std::string(msg));
}

inline tl::expected<std::string, int> ok_str(std::string_view v) {
    return std::string(v);
}
inline tl::expected<std::string, int> err_str(int code) {
    return tl::make_unexpected(code);
}

// NOTE deliberately absent: expected<void, std::string> factories. Reflecting that
// spec is blocked by two toolchain bugs (TC-0007: members_of instantiates member
// bodies, wrong-rejecting the valid void spec; TC-0006: can_substitute on its
// swap<OT=void,...> member template SEGVs the compiler), and a factory return type
// here would drag the spec into the binder's signature-reachability discovery.

// --- the real unexpected->expected converting ctor, driven natively ---
inline tl::expected<int, std::string>
from_unexpected(const tl::unexpected<std::string>& u) {
    return tl::expected<int, std::string>(u);
}

// --- thin wrappers over the all-template comparison front ---
inline bool eq_int(const tl::expected<int, std::string>& a,
                   const tl::expected<int, std::string>& b) {
    return a == b;
}
inline bool eq_int_value(const tl::expected<int, std::string>& a, int v) {
    return a == v;
}
inline bool eq_int_unexpected(const tl::expected<int, std::string>& a,
                              const tl::unexpected<std::string>& u) {
    return a == u;
}

// --- thin wrappers over the member templates with non-defaulted parameters ---
inline int value_or_int(const tl::expected<int, std::string>& a, int alt) {
    return a.value_or(alt);
}
inline tl::expected<int, std::string>
map_double(const tl::expected<int, std::string>& a) {
    return a.map([](int v) { return v * 2; });
}
inline tl::expected<int, std::string>
and_then_halve(const tl::expected<int, std::string>& a) {
    return a.and_then([](int v) -> tl::expected<int, std::string> {
        if (v % 2 != 0)
            return tl::make_unexpected(std::string("odd"));
        return v / 2;
    });
}

}  // namespace extest
