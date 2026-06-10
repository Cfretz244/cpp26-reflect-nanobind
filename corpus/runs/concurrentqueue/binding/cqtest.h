// cqtest -- minimal observability fixture for the moodycamel concurrentqueue run.
//
// The REAL queue (moodycamel::ConcurrentQueue<T>) is bound head-on: its non-template
// enqueue/try_enqueue overloads, size_approx(), and is_lock_free() are ordinary member
// functions and bind directly. The one thing Python cannot drive head-on is dequeue:
// ConcurrentQueue::try_dequeue(U& item) is (a) a MEMBER TEMPLATE (binder skips member
// templates, BINDER-0001) and (b) returns its result through an OUT-PARAMETER reference,
// which has no Python representation. This fixture wraps ONLY that dequeue call,
// re-expressing the out-param success/value as a std::optional<T> the nanobind
// optional caster covers. It owns nothing and adds no queue behaviour of its own --
// every enqueue, size, and ordering decision still goes through the real bound queue;
// the fixture is a pure read adapter over real_queue.try_dequeue.
#pragma once

#include <concurrentqueue.h>

#include <optional>
#include <string>

namespace cqtest {

// Dequeue one element from a real ConcurrentQueue<int>, returning nullopt when empty.
inline std::optional<int> try_dequeue_int(moodycamel::ConcurrentQueue<int>& q) {
    int item = 0;
    if (q.try_dequeue(item)) return item;
    return std::nullopt;
}

inline std::optional<std::string>
try_dequeue_str(moodycamel::ConcurrentQueue<std::string>& q) {
    std::string item;
    if (q.try_dequeue(item)) return item;
    return std::nullopt;
}

}  // namespace cqtest
