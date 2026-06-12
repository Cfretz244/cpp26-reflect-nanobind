// GCC-2 (EXPECTED TO FAIL on GCC — and that is CONFORMING; reclassified
// 2026-06-12, see corpus/findings/repros/GCC-0002/UPSTREAM.md): a non-static
// constexpr LOCAL variable as the range of a `template for (constexpr ...)`
// is ill-formed under [stmt.expand]/5.2 — the synthesized range variable is
// `constexpr decltype(auto) range = ( expansion-initializer );`, a constexpr
// REFERENCE needing an address constant, which a non-static local lacks.
// GCC's note even carries the fix-it ("add 'static'"; verified working).
// Inline (prvalue) ranges and variable templates work — static storage.
// clang-p2996 ACCEPTS this program; that is the divergence (accepts-invalid).
//
// The binder's emit_indices_v variable template (nb_reflect_emit.h) is the
// correct portable spelling, not a workaround for a GCC defect.
#include <meta>
#include <cstdio>
#include <vector>

using namespace std::meta;

consteval std::vector<int> iota_vec(int n) {
    std::vector<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    return v;
}

template <int N>
int sum_indices() {
    int s = 0;
    constexpr auto indices = std::define_static_array(iota_vec(N));
    template for (constexpr auto i : indices) {   // GCC: 'indices' is not a constant expression
        s += i;
    }
    return s;
}

int main() {
    printf("sum=%d\n", sum_indices<4>());
}
