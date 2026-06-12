// String churn with SSO only (no heap allocation): isolates non-heap garbage.
#include <string>
#ifndef N
#define N 100000
#endif
consteval std::size_t churn() {
    std::size_t total = 0;
    for (int i = 0; i < N; ++i) {
        std::string s = "ab";   // SSO, no allocation
        s += 'c';
        total += s.size();
    }
    return total;
}
static_assert(churn() > 0);
