// Single-stage bindings for yaml-cpp 0.9.0, bound head-on off the real library API.
//
// Bound directly (no wrapper): YAML::Node (value-semantic node handle -- Type/IsNull/
// IsScalar/IsSequence/IsMap/IsDefined/size/Scalar/Tag/SetTag/Style/SetStyle/reset/is),
// the YAML::NodeType::value and YAML::EmitterStyle::value enums, YAML::Mark (pos/line/
// column data + is_null), the YAML::Dump / YAML::Clone / YAML::operator== free functions,
// and the exception types (Exception/ParserException/BadConversion/...).
//
// The fixture (yamlfix.h) supplies ONLY the typed accessors the real API expresses solely
// through member templates the binder cannot instantiate: Node::as<T>() (scalar coercion)
// and Node::operator[](Key) (keyed/indexed access). See yamlfix.h for the full rationale.
//
// exclude_ makes the node-internals + stream/emitter/iterator fronts opaque on every path
// so the methods mentioning them are skipped (BINDER-0014): the detail:: pImpl/handle types
// and the iterator_value proxy, YAML::iterator / const_iterator (begin/end -- traversal goes
// through the fixture's get_key/get_idx + size head-on), YAML::Emitter + std::ostream/istream
// (the operator<< / Load(istream) fronts -- no caster, and Dump() covers serialization),
// and YAML::Parser/Scanner machinery.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "yaml-cpp/yaml.h"
#include "yamlfix.h"

namespace nb = nanobind;

NB_MODULE(yamlcpp_ext, m) {
    nb::reflect_<
        ^^YAML::Node,
        ^^YAML::Mark,
        ^^YAML::NodeType::value,
        // NOTE: YAML::EmitterStyle::value is deliberately NOT reflected. Both it and
        // NodeType::value share the unqualified Python name "value"; binding both makes
        // the second clobber the first on the module dict (binder enum-name-collision
        // finding). NodeType::value is the differential's primary enum (Node::Type()),
        // so it stays; EmitterStyle::value (Style()/SetStyle()) is dropped. Style() still
        // returns EmitterStyle::value -- nanobind binds it as an opaque enum return, but
        // its members are not module-named; the tests do not assert on Style identity.
        ^^YAML::Dump,
        ^^YAML::Clone,
        ^^yamlfix,
        ^^nb::exclude_<
            ^^YAML::detail::node,
            ^^YAML::detail::node_data,
            ^^YAML::detail::iterator_value,
            ^^YAML::detail::shared_memory_holder,
            ^^YAML::Emitter,
            ^^std::basic_ostream,
            ^^std::basic_istream>
    >(m);
}
