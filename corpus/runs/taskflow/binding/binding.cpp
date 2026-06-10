// Single-stage bindings for taskflow v4.0.0 (Tier 4: task-graph engine).
//
// tf::Taskflow / tf::Task / tf::Executor are bound HEAD-ON: their real concrete
// classes, queried for graph shape and execution. The graph-construction front
// (FlowBuilder::emplace / Task::precede) is entirely templated over C++ callables
// and variadic Task packs -- not Python-expressible -- so the tftest.Diamond
// fixture builds a fixed diamond graph (A -> {B,C} -> D) with an execution-order
// log + atomic counter the tests observe, then hands out the REAL Taskflow/Task
// handles and drives a REAL Executor::run. Every differential number comes from a
// taskflow class method, not the fixture.
//
// tftest.h is first: it pulls <bit> before <taskflow/taskflow.hpp> (v4.0.0 uses
// std::bit_ceil/std::bit_width without including <bit>; library missing-include).
#include "tftest.h"

#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

NB_MODULE(taskflow_ext, m) {
  // tf::Taskflow inherits FlowBuilder; FlowBuilder is NOT in the bind set so it
  // flattens (its surface is all templated emplace/build, nothing bindable head-on).
  // exclude_ makes the types the binder cannot express opaque on every path so the
  // members mentioning them are skipped (BINDER-0014):
  //   - tf::Future<void> (Executor::run return; inherits std::future<void>, no caster)
  //   - tf::WorkerInterface (Executor ctor's unique_ptr<WorkerInterface> 2nd param)
  //   - tf::Graph / FlowBuilder (graph internals / the templated build front)
  //   - std::ostream (Taskflow::dump(ostream&) / Task::dump(ostream&) -- the no-arg
  //     dump()->string overload is bound instead)
  nb::reflect_<^^tf::Taskflow, ^^tf::Task, ^^tf::Executor, ^^tftest,
               ^^nb::exclude_<^^tf::Future, ^^tf::WorkerInterface, ^^tf::Graph,
                              ^^tf::FlowBuilder, ^^std::basic_ostream>>(m);
}
