// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// stdout_sink_base<console_mutex> (the intermediate of the sink chain) is
// listed EXPLICITLY: its only signature self-mentions are its deleted
// copy/move operators, and deleted functions pull nothing into the bind set
// (BINDER-0012) -- without the explicit opt-in it would flatten instead of
// being the real Python base the run's inheritance theme pins.
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^spdlog::logger, ^^spdlog::level, ^^spdlog::pattern_time_type,            \
    ^^spdlog::sinks::sink,                                                     \
    ^^spdlog::sinks::stdout_sink_base<spdlog::details::console_mutex>,         \
    ^^spdlog::sinks::stdout_sink_mt, ^^logtest
