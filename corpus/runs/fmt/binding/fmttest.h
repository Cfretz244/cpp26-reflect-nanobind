// Test fixtures for the fmtlib/fmt (11.2.0) corpus run, included by BOTH the bindings module
// (binding.cpp) and shared with the native oracle's expectations.
//
// WHY a fixture namespace: fmt's *entire* public formatting API is FUNCTION TEMPLATES
// (`fmt::format(format_string<Args...>, Args&&...)`, `fmt::format_to`, ...), which the binder
// correctly does not auto-bind (you can bind a template's instantiations, not the template).
// So Python cannot call fmt::format directly. These concrete free-function wrappers pin the
// argument types and drive the REAL fmt::format engine; the differential oracle evaluates the
// identical fmt::format expressions natively, so any divergence is attributable to the binding
// layer (both share one compiler + one fmt). The wrappers are TEST SCAFFOLDING; the value under
// test is the binder-exposed surface (free functions + kwargs + the real fmt enums) and fmt's
// real formatting output.
//
// LIB-0001 workaround: <cstdlib> is included FIRST. fmt's detail::allocator<T> in <fmt/format.h>
// calls unqualified global malloc()/free() without including <cstdlib> and without std::
// qualification; strict C++ two-phase lookup in clang-p2996 rejects the non-dependent unqualified
// call unless the name is declared before the template definition. (See findings/LIB-0001.)
#pragma once
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#include <fmt/ranges.h>   // fmt::join

namespace fmttest {

// --- free-function wrappers over the real fmt::format engine ---------------------------------
// Named parameters (spec/value/width/...) are recovered by the binder via P3096 and emitted as
// nb::arg("name"), so Python can call these with keyword arguments.

// General: a runtime-checked format string (supplied from Python) applied to one value.
inline std::string apply_int(const std::string& spec, std::int64_t value) {
    return fmt::format(fmt::runtime(spec), value);
}
inline std::string apply_double(const std::string& spec, double value) {
    return fmt::format(fmt::runtime(spec), value);
}
inline std::string apply_str(const std::string& spec, const std::string& value) {
    return fmt::format(fmt::runtime(spec), value);
}
inline std::string apply_two(const std::string& spec, std::int64_t a, std::int64_t b) {
    return fmt::format(fmt::runtime(spec), a, b);
}

// Compile-time-checked format strings with meaningful kwarg names.
inline std::string pad_left(std::int64_t value, std::int64_t width) {
    return fmt::format("{:>{}}", value, width);
}
inline std::string hex_upper(std::int64_t value) {
    return fmt::format("{:X}", value);
}

// fmt::join over an STL container (exercises the std::vector caster + a real fmt range feature).
inline std::string join_ints(const std::vector<std::int64_t>& items, const std::string& sep) {
    return fmt::format("{}", fmt::join(items, sep));
}
inline std::string join_strs(const std::vector<std::string>& items, const std::string& sep) {
    return fmt::format("{}", fmt::join(items, sep));
}

// --- member-function-template gap (a concrete fixture class) ---------------------------------
// Demonstrates the binder's documented limitation (BINDER-0001): member FUNCTION TEMPLATES are
// gracefully SKIPPED while ordinary members bind. Here `apply<T>` is a member template (skipped);
// the default ctor, the string ctor, `spec()`, and `apply_int()` are ordinary and bind. All of it
// is backed by the real fmt engine.
struct Formatter {
    std::string spec_;
    Formatter() : spec_("{}") {}
    explicit Formatter(const std::string& spec) : spec_(spec) {}

    std::string spec() const { return spec_; }                        // bound
    std::string apply_int(std::int64_t value) const {                 // bound
        return fmt::format(fmt::runtime(spec_), value);
    }
    template <class T>                                                 // SKIPPED (member template)
    std::string apply(T value) const {
        return fmt::format(fmt::runtime(spec_), value);
    }
};

}  // namespace fmttest
