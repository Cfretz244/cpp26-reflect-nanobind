// Single-stage bindings for yaml-cpp 0.9.0, bound head-on off the real library API.
//
// Bound directly (no wrapper): YAML::Node (value-semantic node handle -- Type/IsNull/
// IsScalar/IsSequence/IsMap/IsDefined/size/Scalar/Tag/SetTag/Style/SetStyle/reset/is),
// the YAML::NodeType::value and YAML::EmitterStyle::value enums (BINDER-0022: same
// unqualified name "value" -- the second binds parent-qualified as "EmitterStyle_value"),
// YAML::Mark (pos/line/column data + is_null), the YAML::Dump / YAML::Clone / YAML::operator==
// free functions, and the exception types (Exception/ParserException/BadConversion/...).
//
// The fixture (yamlfix.h) supplies ONLY the typed accessors the real API expresses solely
// through member templates the binder cannot instantiate: Node::as<T>() (scalar coercion)
// and Node::operator[](Key) (keyed/indexed access). See yamlfix.h for the full rationale.
//
// The bind set + exclude_ pack live in binding_args.h (shared with the emit generator).
#include <nanobind/nb_reflect.h>
#include "binding_args.h"  // bind set defined once (shared with the emit generator)
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

NB_MODULE(yamlcpp_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
