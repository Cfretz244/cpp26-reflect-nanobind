// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
//
// EXCLUSIONS (built programmatically): ConcurrentQueue declares seven
// `static const size_t/uint32_t` configuration constants with in-class
// initializers and NO out-of-line definitions; they predate BINDER-0020's
// bind-by-value path in this run's subset and are excluded from the
// differential on both specs (see the original finding draft).
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"

namespace cq_args_detail {

// Collect the never-defined static const config constants of the two queue
// specs into an nb::exclude_<...> marker so the binder skips binding them.
consteval std::meta::info cq_excluded_marker() {
    namespace sm = std::meta;
    std::vector<std::meta::info> args;
    for (sm::info Q : {^^moodycamel::ConcurrentQueue<int>,
                       ^^moodycamel::ConcurrentQueue<std::string>}) {
        for (auto mem : sm::members_of(Q, sm::access_context::unchecked())) {
            if (nanobind::detail::is_using_proxy(mem)) continue;
            if (sm::is_static_member(mem) && sm::is_variable(mem))
                args.push_back(sm::reflect_constant(mem));
        }
    }
    return sm::substitute(^^nanobind::exclude_, args);
}

}  // namespace cq_args_detail

#define CORPUS_REFLECT_ARGS                                                    \
    ^^moodycamel::ConcurrentQueue<int>,                                        \
    ^^moodycamel::ConcurrentQueue<std::string>,                                \
    cq_args_detail::cq_excluded_marker(),                                      \
    ^^cqtest
