// Single-stage bindings for toml++ v3.4.0.
//
// What is bound HEAD-ON (the library's own real types):
//   toml::node_type      -- the value-kind discriminant enum (all 10 values)
//   toml::date           -- year/month/day  (its comparison operators are hidden friends,
//                           not namespace members, so they are not reflected -- data only)
//   toml::time           -- hour/minute/second/nanosecond
//   toml::time_offset    -- minutes
//   toml::date_time      -- date/time/offset (+ is_local())
//   toml::source_position-- line/column
//   toml::source_region  -- begin/end  (path excluded: shared_ptr<const std::string>)
//   toml::parse_error    -- description()/source()/what()
//
// What is NOT bound, and why (see findings_draft/binder-lvalue-ref-to-pointer-param.md):
//   toml::node / table / array / value<T> / parse_result -- every node class declares
//   `virtual bool is_homogeneous(node_type, node*& first_nonmatch) noexcept;` and the binder
//   hard-errors on the `node*&` out-param rather than skipping it (per-overload nb::exclude_
//   is ineffective for these). Tree navigation therefore runs in C++ (tomlfix) and surfaces
//   plain Python values plus the bound date/time structs above.
//
// The tomlfix namespace parses + walks the tree and hands back results using the library's
// own parse/coerce/format code (see tomlfix.h). The differential drives the SAME document
// natively and from Python and compares real bound objects (date/time/date_time fields,
// node_type enum values, parse_error line/column) plus coerced scalars and round-trip text.
// Bind set defined once in binding_args.h (shared with the emit-lane generator).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/vector.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(tomlpp_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
