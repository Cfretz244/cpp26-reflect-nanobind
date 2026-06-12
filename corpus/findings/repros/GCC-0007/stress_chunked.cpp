// GCC-0007: same total churn as stress_churn.cpp but split into CHUNKS
// top-level constant evaluations (template-parameterized static_asserts).
// If cc1plus collected garbage between top-level evaluations, peak RSS
// would track ONE chunk; observed: it tracks the TOTAL across chunks.
#include <string>

#ifndef N
#define N 10000
#endif
#ifndef CHUNKS
#define CHUNKS 20
#endif

consteval std::string int_str(int v) {
    std::string out;
    do { out.insert(out.begin(), char('0' + v % 10)); v /= 10; } while (v);
    return out;
}

template <int Lo, int Hi>
consteval std::size_t churn_range() {
    std::size_t total = 0;
    for (int i = Lo; i < Hi; ++i) {
        std::string s = "chunk_" + int_str(i) + "_0123456789abcdef";
        total += s.size();
    }
    return total;
}

template <int K>
struct Chunk {
    static_assert(churn_range<K * (N / CHUNKS), (K + 1) * (N / CHUNKS)>() > 0);
    static constexpr bool ok = true;
};

template <int... Ks>
consteval bool run_all(std::integer_sequence<int, Ks...>) {
    return (Chunk<Ks>::ok && ...);
}
static_assert(run_all(std::make_integer_sequence<int, CHUNKS>{}));
