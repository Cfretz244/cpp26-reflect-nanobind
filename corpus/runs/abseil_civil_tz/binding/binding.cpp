// Binding TU for the Abseil civil-time/TimeZone run. The civil types are REAL
// internal-namespace template specializations (CivilDay = time_internal::cctz::
// detail::civil_time<day_tag>) bound under their spec-derived Python names;
// TimeZone::CivilInfo is a nested class listed explicitly (nested non-template
// classes are not auto-discovered); Time/Duration are plain classes likewise
// listed (CivilInfo's members reference them).
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/optional.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(abseil_civil_tz_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
