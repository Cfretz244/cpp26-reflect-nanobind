// Single-stage bindings for CLI11 v2.6.2 (Tier 3: command-line parser).
//
// CLI::App is the central class, bound head-on. The bindable, non-templated surface is the
// string-collecting option API: add_option(name[, desc]) and add_flag(name[, desc]) return
// an Option* whose parsed values are kept as strings (results_t == vector<string>) -- this
// is exactly the path that does NOT need a typed C++ variable, so it crosses to Python with
// no fixture. parse(vector<string>&) drives a parse from an argv-style vector, avoiding argc/
// argv. CLI::Option exposes count()/empty()/results()/get_name and the configuration
// chain (required/expected). The Error hierarchy (ParseError & co.) is reflected so the
// failure-path differential can see real CLI exception types.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/wstring.h>
#include <nanobind/stl/optional.h>

#include <CLI/CLI.hpp>

namespace nb = nanobind;
namespace sm = std::meta;

namespace {

// A method parameter that is a pointer-to-pointer (char**, const char* const*,
// const wchar_t* const*) has no nanobind caster, and the binder does not gracefully
// skip it -- it synthesizes a forwarding lambda whose signature nanobind then fails
// to match, a hard compile error (see findings_draft/binder-ptr-to-ptr-param).
// App's argv-style parse(int, char**) / parse(int, wchar_t**) overloads and
// ensure_utf8(char**) are exactly that shape; the bindable parse(vector<string>&)
// is what the run actually drives, so excluding these by member reflection keeps
// the head-on App surface intact. (collected, not a whole-type/template exclude,
// because the trigger is a builtin pointer type, not a nameable entity.)
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
            if (!nb::detail::is_using_proxy(mem) && sm::is_function(mem) &&
                !sm::is_template(mem) && has_ptr_to_ptr_param(mem))
                add(mem);
        }
    }
    return sm::substitute(^^nb::exclude_, args);
}

}  // namespace

NB_MODULE(cli11_ext, m) {
    nb::reflect_<^^CLI::App, ^^CLI::Option, ^^CLI::Error, ^^CLI::ParseError,
                 ^^CLI::RequiredError, ^^CLI::ExtrasError, ^^CLI::ArgumentMismatch,
                 ^^CLI::ConversionError,
                 cli_excluded_marker()>(m);
}
