// Single-stage bindings for moodycamel concurrentqueue v1.0.5 (Tier 3).
//
// The library's central class moodycamel::ConcurrentQueue<T> is bound head-on for two
// element types (int and std::string), as explicit specs in the reflect_ pack. Its
// non-template enqueue / try_enqueue overloads, size_approx(), and is_lock_free() bind
// directly. The dequeue side -- try_dequeue(U&), a member template returning through an
// out-parameter -- is the one thing Python cannot drive head-on, so the cqtest namespace
// supplies a thin optional<T>-returning adapter over the real queue's try_dequeue
// (see cqtest.h for the justification).
//
// EXCLUSIONS (nb::exclude_, built programmatically below): ConcurrentQueue declares seven
// `static const size_t/uint32_t` configuration constants (BLOCK_SIZE, MAX_SUBQUEUE_SIZE,
// EXPLICIT_INITIAL_INDEX_SIZE, IMPLICIT_INITIAL_INDEX_SIZE, INITIAL_IMPLICIT_PRODUCER_HASH_SIZE,
// EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD, EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE)
// with in-class initializers but NO out-of-line definitions. The binder binds static data
// members by taking their address (&Class::MEMBER), which ODR-uses them and so emits an
// undefined symbol at link (pre-C++17 in-class-initialized static const integrals are usable
// as constants but have no storage unless defined out-of-line, which moodycamel never does).
// These constants are not part of the producer/consumer differential, so they are excluded
// here on both specs; see findings_draft/binder-static-const-member-address-odr-use.md.
// Bind set defined once in binding_args.h (shared with the emit-lane generator).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/optional.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(concurrentqueue_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
