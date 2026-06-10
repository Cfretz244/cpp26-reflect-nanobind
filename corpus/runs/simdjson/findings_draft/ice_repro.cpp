// Minimal repro: deduction-guide mangler ICE when the binder's bind-set fixpoint reaches a class
// that (a) derives from std::pair and (b) lives in a namespace carrying deduction guides, while
// that class is named in nb::exclude_. Trigger isolated from the simdjson DOM run: reflecting a
// namespace that declares a namespace-alias to the whole library let the fixpoint discover
// simdjson::arm64::ondemand::field (: std::pair<raw_json_string, value>); excluding ^^field then
// drove ItaniumMangle.cpp:1774 `llvm_unreachable("Can't mangle a deduction guide name!")`.
//
// compile: see header of the run; the same flags as binding.cpp. EXPECTED: clean compile (or a
// graceful skip). ACTUAL: clang frontend exit 134, UNREACHABLE at ItaniumMangle.cpp:1774.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include "simdjson.h"

namespace nb = nanobind;

// A reflected fixture namespace whose namespace-alias member leaks the whole library to the walk.
namespace leaky {
namespace sd = simdjson;          // <-- alias member; reflecting ^^leaky enumerates it
inline simdjson::dom::element root(simdjson::dom::parser& p, const std::string& s) {
  return p.parse(s).value();
}
}

NB_MODULE(ice_repro, m) {
    nb::reflect_<
        ^^simdjson::dom::element,
        ^^leaky,
        ^^nb::exclude_<^^simdjson::simdjson_result, ^^simdjson::dom::parser,
                       ^^simdjson::dom::document, ^^simdjson::dom::array,
                       ^^simdjson::dom::object, ^^simdjson::internal::tape_ref,
                       ^^simdjson::ondemand::field,   // <-- excluding this drives the guide-mangle ICE
                       ^^std::basic_ostream, ^^std::vector, ^^std::pair>
    >(m);
}
