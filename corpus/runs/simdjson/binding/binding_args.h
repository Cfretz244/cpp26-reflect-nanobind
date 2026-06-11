// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The exclude_ set makes everything the DOM mentions that Python cannot
// represent opaque (BINDER-0014): the simdjson_result monad (every
// nav/coerce return), the move-only parser/document/document_stream fronts,
// key_value_pair, the padded-string parse fronts, the private tape_ref
// handle, and the ostream sink. std::vector is NOT excluded -- the fixture's
// array_values/object_keys return std::vector<element>/<string> (covered by
// <nanobind/stl/vector.h>).
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^simdjson::dom::element,                                                  \
    ^^simdjson::dom::array,                                                    \
    ^^simdjson::dom::object,                                                   \
    ^^simdjson::dom::element_type,                                             \
    ^^simdjson::error_code,                                                    \
    ^^simdjsontest,                                                            \
    ^^nanobind::exclude_<                                                      \
        ^^simdjson::simdjson_result,                                           \
        ^^simdjson::dom::parser,                                               \
        ^^simdjson::dom::document,                                             \
        ^^simdjson::dom::document_stream,                                      \
        ^^simdjson::dom::key_value_pair,                                       \
        ^^simdjson::padded_string,                                             \
        ^^simdjson::padded_string_view,                                        \
        ^^simdjson::internal::tape_ref,                                        \
        ^^std::basic_ostream>
