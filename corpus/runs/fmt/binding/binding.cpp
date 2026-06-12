// Single-stage bindings for fmtlib/fmt (11.2.0). Four layers of real-fmt surface:
//
//   1. fmt's real ENUMS (color / terminal_color / emphasis) — reflected directly.
//   2. fmt's real CONCRETE CLASS `fmt::format_int` (fmt's fast integer->string formatter) —
//      reflected directly: its constructor overloads (int/long/long long/unsigned/...) and
//      str()/c_str()/data()/size() bind; its two private member templates are skipped.
//   3. fmt's real FORMATTING API — `fmt::to_string<T>` is a function template; we list concrete
//      instantiations (`^^fmt::to_string<int/double/long long>`) in the reflect pack so the binder
//      binds fmt's OWN symbol (not a wrapper). These format real values through fmt. (fmt's
//      `format`/`print` front-ends take a *consteval* `format_string<Args...>` first parameter that
//      cannot be constructed at runtime from Python, so they are not directly bindable —
//      `to_string<T>`/`format_int` are the directly-bindable formatting entry points. The runtime
//      vformat path is exercised separately via the fmttest wrappers below.)
//   4. The fmttest fixture namespace — concrete wrappers over the runtime `fmt::format` engine
//      that drive the remaining Tier-2 themes (kwargs, std::string/std::vector casters incl.
//      fmt::join, and the member-function-template gap). See fmttest.h.
//
// No virtuals in the reflected set, so the single-stage path is used; the STL caster headers the
// signatures need (std::string, std::vector) are included explicitly (the header-only reflect_
// path can't emit includes — it static_asserts on a missing one).
//
// NOTE: the to_string<T> instantiations bind under template-spec CamelCase names with an
// enable_if-NTTP suffix (`to_stringInt0`, `to_stringDouble0`, `to_stringLonglong0`) — the open
// cosmetic BINDER-0003.
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(fmt_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
