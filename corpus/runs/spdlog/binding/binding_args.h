// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The sink family is selected by a derived_from_ match over spdlog::sinks:
// the leaf accepts the abstract base itself plus every visible concrete sink
// (the namespace-level *_mt/_st aliases are dealiased to their template
// specializations, exactly what listing them explicitly produced -- the
// unchanged stdout_sinkConsole_mutex tests are the equivalence proof), and
// not_<named_<"ostream_sink*">> keeps the documented skip (its ctor takes
// std::ostream& -- no caster; the logtest CaptureSink wraps it instead).
// stdout_sink_base<console_mutex> (the intermediate of the sink chain) is
// still listed EXPLICITLY: it is a template specialization the namespace
// walk cannot reach, and its only signature self-mentions are its deleted
// copy/move operators, which pull nothing into the bind set (BINDER-0012) --
// without the explicit opt-in it would flatten instead of being the real
// Python base the run's inheritance theme pins. color_mode is seeded so the
// ansicolor ctors are Python-callable (their default argument VALUE is the
// P3096 gap, passed explicitly in tests).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^spdlog::logger, ^^spdlog::level, ^^spdlog::pattern_time_type,            \
    ^^spdlog::color_mode,                                                      \
    ^^mirrorbind::match_<^^spdlog::sinks,                                        \
        mirrorbind::all_of_<                                                     \
            mirrorbind::derived_from_<^^spdlog::sinks::sink>,                     \
            mirrorbind::not_<mirrorbind::named_<"ostream_sink*">>>>,              \
    ^^spdlog::sinks::stdout_sink_base<spdlog::details::console_mutex>,         \
    ^^logtest
