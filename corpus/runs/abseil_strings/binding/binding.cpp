// Binding TU for the Abseil strings run: absl::Cord bound head-on (ctors incl.
// explicit Cord(string_view), Append/Prepend non-template overloads,
// RemovePrefix/RemoveSuffix/Subcord, size/empty/Clear, Compare/StartsWith/
// EndsWith, TryFlat -> optional<string_view>, operator<< -> __str__), plus the
// strtest fixture wrappers over the variadic-template free-function API.
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(abseil_strings_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
