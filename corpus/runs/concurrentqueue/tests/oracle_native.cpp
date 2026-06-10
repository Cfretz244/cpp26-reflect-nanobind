// Native C++ ground-truth oracle for the concurrentqueue binding (Layer-1 differential).
// Drives the EXACT single-threaded enqueue/dequeue sequence the Python test drives through
// the bound module -- same element values, same partial drain, same empty-dequeue probe --
// via the real moodycamel::ConcurrentQueue and the shared cqtest dequeue adapter, and emits
// every observable as JSON. Shared compiler + shared (header-only) queue => any divergence
// is the binding layer's.
#include "../binding/cqtest.h"

#include <concurrentqueue.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto add_arr = [&](const char* k, const std::vector<std::string>& xs) {
        std::string s = "[";
        for (size_t i = 0; i < xs.size(); ++i) { if (i) s += ","; s += xs[i]; }
        s += "]";
        kv.emplace_back(k, s);
    };

    // --- int queue: single (implicit) producer FIFO scenario ---
    moodycamel::ConcurrentQueue<int> q(64);

    add_b("is_lock_free", moodycamel::ConcurrentQueue<int>::is_lock_free());
    add_i("size_empty", static_cast<std::int64_t>(q.size_approx()));

    // empty dequeue => nullopt
    add_b("empty_dequeue_has_value", cqtest::try_dequeue_int(q).has_value());

    // enqueue 0..9 head-on through the real queue
    bool all_enq = true;
    for (int i = 0; i < 10; ++i) all_enq = q.enqueue(i) && all_enq;
    add_b("all_enqueued", all_enq);
    add_i("size_after_enqueue", static_cast<std::int64_t>(q.size_approx()));

    // try_enqueue (non-allocating) one more
    add_b("try_enqueue_ok", q.try_enqueue(100));
    add_i("size_after_try", static_cast<std::int64_t>(q.size_approx()));

    // drain first 5 -- single producer => FIFO
    std::vector<std::string> first5;
    for (int i = 0; i < 5; ++i) {
        auto v = cqtest::try_dequeue_int(q);
        first5.push_back(v ? std::to_string(*v) : "null");
    }
    add_arr("first5", first5);
    add_i("size_after_drain5", static_cast<std::int64_t>(q.size_approx()));

    // drain the rest
    std::vector<std::string> rest;
    while (true) {
        auto v = cqtest::try_dequeue_int(q);
        if (!v) break;
        rest.push_back(std::to_string(*v));
    }
    add_arr("rest", rest);
    add_i("size_drained", static_cast<std::int64_t>(q.size_approx()));
    add_b("empty_again", !cqtest::try_dequeue_int(q).has_value());

    // --- string queue: value round-trip ---
    moodycamel::ConcurrentQueue<std::string> qs(8);
    qs.enqueue(std::string("alpha"));
    qs.enqueue(std::string("beta"));
    qs.try_enqueue(std::string("gamma"));
    std::vector<std::string> strs;
    while (true) {
        auto v = cqtest::try_dequeue_str(qs);
        if (!v) break;
        strs.push_back("\"" + *v + "\"");
    }
    add_arr("strs", strs);

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
