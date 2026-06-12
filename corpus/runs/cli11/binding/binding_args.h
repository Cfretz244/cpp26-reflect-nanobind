// The reflect_ pack for the CLI11 run, defined ONCE for every backend consumer
// (constexpr lane's binding.cpp + emit lane's gen_emit.cpp). P2996-only: this
// header carries the `^^`/consteval marker machinery, so it must be compiled by
// the reflection toolchain (NEVER the production compiler). The plain C++
// library includes live in binding_includes.h.
#pragma once

#include <mirrorbind/reflect.h>
#include "binding_includes.h"

namespace nb = nanobind;
namespace mb = mirrorbind;
namespace sm = std::meta;

namespace cli_corpus {

// A method parameter that is a pointer-to-pointer (char**, const char* const*,
// const wchar_t* const*) has no nanobind caster, and the binder does not
// gracefully skip it -- it synthesizes a forwarding lambda whose signature
// nanobind then fails to match, a hard compile error (see
// findings_draft/binder-ptr-to-ptr-param-no-graceful-skip.md). App's argv-style
// parse(int, char**) / parse(int, wchar_t**) overloads and ensure_utf8(char**)
// are exactly that shape; the bindable parse(vector<string>&) is what the run
// drives, so excluding these by member reflection keeps the head-on App surface
// intact. (collected, not a whole-type/template exclude, because the trigger is
// a builtin pointer type, not a nameable entity.)
consteval bool has_ptr_to_ptr_param(sm::info fn) {
    for (sm::info p : sm::parameters_of(fn)) {
        sm::info t = sm::remove_cvref(sm::type_of(p));
        if (sm::is_pointer_type(t)) {
            sm::info inner = sm::remove_cvref(sm::remove_pointer(t));
            if (sm::is_pointer_type(inner))
                return true;
        }
    }
    return false;
}

consteval sm::info cli_excluded_marker() {
    std::vector<sm::info> args;
    auto add = [&](sm::info e) { args.push_back(sm::reflect_constant(e)); };

    for (sm::info owner : {^^CLI::App, ^^CLI::Option}) {
        for (sm::info mem : sm::members_of(owner, sm::access_context::unchecked())) {
            if (sm::is_function(mem) &&
                !sm::is_template(mem) && has_ptr_to_ptr_param(mem))
                add(mem);
        }
    }
    return sm::substitute(^^mb::exclude_, args);
}

}  // namespace cli_corpus

// CLI::App is bound head-on through its string-collecting option API; the
// CLI::ParseError hierarchy is reflected for the failure-path differential. The
// excluded-marker drops the pointer-to-pointer overloads (see above).
#define CORPUS_REFLECT_ARGS                                                   \
    ^^CLI::App, ^^CLI::Option, ^^CLI::Error, ^^CLI::ParseError,               \
    ^^CLI::RequiredError, ^^CLI::ExtrasError, ^^CLI::ArgumentMismatch,        \
    ^^CLI::ConversionError, cli_corpus::cli_excluded_marker()
