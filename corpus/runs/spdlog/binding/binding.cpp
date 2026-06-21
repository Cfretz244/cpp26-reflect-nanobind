// Single-stage bindings for spdlog v1.17 (Tier 3: inheritance + shared_ptr + level enums).
// logger bound head-on; spdlog::level namespace supplies the enum + to_string_view/from_str;
// sinks::sink (abstract base) + sinks::stdout_sink_mt (concrete; its unbound intermediate
// stdout_sink_base<console_mutex> flattens, sink wires through as the Python base); the
// logtest CaptureSink fixture makes logger output observable from Python.
// logtest.h comes first: it selects SPDLOG_USE_STD_FORMAT before any spdlog header.
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/unique_ptr.h>   // formatter::clone() returns std::unique_ptr<formatter> (discovered)
#include <nanobind/stl/function.h>
#include <nanobind/stl/chrono.h>

#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(spdlog_ext, m) {
    // stdout_sink_base<console_mutex> (the intermediate of the sink chain) is
    // listed EXPLICITLY: its only signature self-mentions are its deleted
    // copy/move operators, and deleted functions pull nothing into the bind
    // set (BINDER-0012) -- without the explicit opt-in it would flatten
    // instead of being the real Python base the run's inheritance theme pins.
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
