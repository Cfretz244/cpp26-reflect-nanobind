// Single-stage bindings for CLI11 v2.6.2 (Tier 3: command-line parser).
//
// CLI::App is the central class, bound head-on. The bindable, non-templated
// surface is the string-collecting option API: add_option(name[, desc]) and
// add_flag(name[, desc]) return an Option* (owned by App through
// Option_p = std::unique_ptr<Option>; BINDER-0017 gives such borrowed raw
// returns a reference/reference_internal policy rather than take_ownership).
// parse(vector<string>&) drives a parse from an argv-style vector. CLI::Option
// exposes count()/empty()/results()/get_name and the configuration chain
// (required/expected). The Error hierarchy is reflected for the failure-path
// differential. Bind set + the pointer-to-pointer exclude marker are defined
// ONCE in binding_args.h (shared with the emit-lane generator); this TU adds
// only the STL caster headers the generated bindings reference.
#include <nanobind/nb_reflect.h>
#include "binding_args.h"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/wstring.h>
#include <nanobind/stl/optional.h>

NB_MODULE(cli11_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
