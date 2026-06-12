// GCC-0007 minimized stress: constant-evaluation peak memory scales with the
// TOTAL allocation churn of the evaluation, not with live data. No reflection
// involved -- plain consteval string building, the emit generator's workload
// shape (nb::write_bindings renders a complete binding TU as text in one
// top-level constant evaluation).
//
//   g++ -std=c++26 -DN=50000 -fconstexpr-ops-limit=1000000000 \
//       -fsyntax-only stress_churn.cpp
//
// Live data at any moment: one small std::string (< 64 bytes of payload).
// Total churn: N transient heap-allocated strings. Peak cc1plus RSS should be
// ~O(live); observed: O(total churn), ~95 KB retained per iteration (trunk
// 17.0 20260612 and 16.1 alike), e.g. N=50000 -> ~4.8 GB peak RSS, N=100000
// -> memory exhaustion on a 31 GB machine. clang's evaluator runs the
// equivalent workload in a small constant footprint.
#include <string>

#ifndef N
#define N 50000
#endif

// Local digits-to-string (16.1's libstdc++ has no constexpr std::to_string).
consteval std::string int_str(int v) {
    std::string out;
    do { out.insert(out.begin(), char('0' + v % 10)); v /= 10; } while (v);
    return out;
}

consteval std::size_t churn() {
    std::size_t total = 0;
    for (int i = 0; i < N; ++i) {
        std::string s = "chunk_" + int_str(i) + "_0123456789abcdef";
        total += s.size();
    }
    return total;
}

static_assert(churn() > 0);
