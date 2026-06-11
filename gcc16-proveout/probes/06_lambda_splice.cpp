// The binder's hottest pattern: generic lambdas whose BODIES call through
// spliced member reflections, and spliced types appearing in lambda
// SIGNATURES (the clang-p2996 mangler crash the binder works around — does
// GCC handle it?). Plus member pointers via splice and operator binding.
#include <meta>
#include <cstdio>

namespace api {
struct Vec {
    double x = 1, y = 2;
    double dot(const Vec& o) const { return x * o.x + y * o.y; }
    Vec operator+(const Vec& o) const { return {x + o.x, y + o.y}; }
};
} // namespace api

using namespace std::meta;

consteval info find_method(info cls, std::string_view name) {
    for (info m : members_of(cls, access_context::unchecked()))
        if (is_function(m) && has_identifier(m) && identifier_of(m) == name)
            return m;
    return info{};
}

int main() {
    constexpr info dot = find_method(^^api::Vec, "dot");

    // 1. spliced call in lambda body (always needed)
    auto f = [](const api::Vec& self, const api::Vec& o) {
        return self.[:dot:](o);
    };

    // 2. spliced TYPE in the lambda signature (crashes clang-p2996's mangler;
    //    the binder works around it everywhere — unnecessary under GCC?)
    auto g = [](typename [:^^api::Vec:] self, typename [:return_type_of(dot):] scale) {
        return self.x * scale;
    };

    // 3. operator reflection spliced in a lambda
    constexpr info plus = [] consteval {
        for (info m : members_of(^^api::Vec, access_context::unchecked()))
            if (is_operator_function(m) && operator_of(m) == operators::op_plus)
                return m;
        return info{};
    }();
    auto h = [](const api::Vec& a, const api::Vec& b) { return a.[:plus:](b); };

    api::Vec v1, v2{3, 4};
    double r1 = f(v1, v2);          // 1*3 + 2*4 = 11
    double r2 = g(v1, 2.0);         // 2
    api::Vec r3 = h(v1, v2);        // {4, 6}
    if (r1 == 11 && r2 == 2 && r3.y == 6) printf("06_lambda_splice PASS\n");
    else { printf("06_lambda_splice FAIL %f %f %f\n", r1, r2, r3.y); return 1; }
}
