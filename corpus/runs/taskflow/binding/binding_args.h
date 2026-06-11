// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// exclude_ makes the types the binder cannot express opaque on every path
// (BINDER-0014): tf::Future<void> (inherits std::future<void>, no caster),
// tf::WorkerInterface (unique_ptr ctor param), tf::Graph / tf::FlowBuilder
// (graph internals / the templated build front), std::basic_ostream (the
// dump(ostream&) overloads; the no-arg dump()->string binds instead).
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^tf::Taskflow, ^^tf::Task, ^^tf::Executor, ^^tftest,                      \
    ^^nanobind::exclude_<^^tf::Future, ^^tf::WorkerInterface, ^^tf::Graph,     \
                         ^^tf::FlowBuilder, ^^std::basic_ostream>
