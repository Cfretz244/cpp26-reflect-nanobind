// Single-stage bindings for simdjson v4.6.4 (Tier 3: a value-or-error DOM, template-heavy).
//
// Bound HEAD-ON (the library's own concrete types):
//   dom::element  -- the JSON node handle: type(), all is_*() discriminants, operator==/<
//   dom::array    -- size()
//   dom::object   -- size()
//   dom::element_type (enum) -- ARRAY/OBJECT/INT64/UINT64/DOUBLE/STRING/BOOL/NULL_VALUE/BIGINT
//   error_code    (enum) -- the full parse/navigation error taxonomy
//
// The simdjson DOM is a value-or-error monad: navigation/coercion methods return
// simdjson_result<T> (a template with no caster), and the entry points hang off a MOVE-ONLY
// parser whose results alias parser-owned buffers. Neither is expressible to Python, so those
// surfaces are reached through the simdjsontest fixture (parse -> Doc, then unwrapped navigation/
// coercion returning the SAME real element/array/object types bound here). The exclude_ pack
// makes the unrepresentable types opaque on every discovery path (BINDER-0014) so the head-on
// classes bind cleanly with their result-free members.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace dom = simdjson::dom;

NB_MODULE(simdjson_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
