// GCC-2 (EXPECTED TO FAIL on GCC 16.1; minimized bug repro): a constexpr
// LOCAL variable is rejected as the range of an expansion statement inside a
// function template ("'indices' is not a constant expression"); the same
// initializer expression inline, or hoisted to a variable template, works.
//
// The binder's workaround: emit_indices_v variable template
// (nb_reflect_emit.h).
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
