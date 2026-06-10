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

#include "simdjsontest.h"

namespace nb = nanobind;
namespace dom = simdjson::dom;

NB_MODULE(simdjson_ext, m) {
    nb::reflect_<
        ^^simdjson::dom::element,
        ^^simdjson::dom::array,
        ^^simdjson::dom::object,
        ^^simdjson::dom::element_type,
        ^^simdjson::error_code,
        ^^simdjsontest,
        // Opaque: everything the DOM mentions that Python cannot represent, so the head-on
        // classes' result/iterator/parser-returning members are skipped (BINDER-0014).
        ^^nb::exclude_<
            ^^simdjson::simdjson_result,            // the value-or-error monad (every nav/coerce return)
            ^^simdjson::dom::parser,                // move-only entry point; results alias its buffers
            ^^simdjson::dom::document,              // move-only; unique_ptr members
            ^^simdjson::dom::document_stream,       // many-document streaming front
            ^^simdjson::dom::key_value_pair,        // object iterator value_type
            ^^simdjson::padded_string,              // parse input front (fixture takes std::string)
            ^^simdjson::padded_string_view,
            ^^simdjson::internal::tape_ref,         // private tape handle in every DOM class
            ^^std::basic_ostream                     // dump_raw_tape / operator<< sink
            // NOTE: std::vector is NOT excluded -- the fixture's array_values/object_keys return
            // std::vector<element>/<string> (covered by <nanobind/stl/vector.h>). The DOM classes'
            // own vector fronts (at_path_with_wildcard -> simdjson_result<vector<element>>) are
            // already cut by excluding simdjson_result; get_values returns vector<element>& (an
            // lvalue ref to its out-param) and binds harmlessly.
        >
    >(m);
}
