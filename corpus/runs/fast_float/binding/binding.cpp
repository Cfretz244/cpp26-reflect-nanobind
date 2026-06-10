// Single-stage bindings for fast_float v8.2.8 (Tier 2: free-function parser front + small
// value types). The library's OWN bindable types are reflected head-on:
//   - fast_float::chars_format          : the parse-format bitset enum (general/json/fortran/
//                                          hex/scientific/fixed/...), differentially checked.
//   - fast_float::parse_options_t<char> : the options struct (format/decimal_point/base) with
//                                          its defaulting ctor -- drives from_chars_advanced.
//   - fast_float::from_chars_result_t<char>: the {ptr, ec} result struct + operator bool.
// The actual parse entry points are free function templates over pointer pairs + an out-param
// reference (no Python representation), so the fixture namespace wraps the four concrete
// instantiations (double/float/int64/uint64) as string-in -> {value, consumed, ok, ec}-out.
#include "fixture.h"

#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(fast_float_ext, m) {
    nb::reflect_<^^fast_float::chars_format,
                 ^^fast_float::parse_options_t<char>,
                 ^^fast_float::from_chars_result_t<char>,
                 ^^fixture>(m);
}
