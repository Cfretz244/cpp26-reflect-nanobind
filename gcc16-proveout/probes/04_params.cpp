// Parameter reflection (P3096): parameters_of + identifier_of on params,
// has_default_argument — drives the binder's kwarg naming (nb::arg).
#include <meta>
#include <cstdio>

namespace api {
struct Calc {
    int add(int lhs, int rhs) { return lhs + rhs; }
    int scale(int value, int factor = 2) { return value * factor; }
};
int free_fn(double amount, bool round_up) { return 0; }
} // namespace api

using namespace std::meta;

int main() {
    constexpr auto ps = define_static_array(parameters_of(^^api::free_fn));
    static_assert(ps.size() == 2);
    static_assert(has_identifier(ps[0]) && identifier_of(ps[0]) == "amount");
    static_assert(identifier_of(ps[1]) == "round_up");
    static_assert(is_same_type(type_of(ps[0]), ^^double));

    constexpr info scale = [] consteval {
        for (info m : members_of(^^api::Calc, access_context::unchecked()))
            if (is_function(m) && has_identifier(m) && identifier_of(m) == "scale")
                return m;
        return info{};
    }();
    constexpr auto sps = define_static_array(parameters_of(scale));
    static_assert(sps.size() == 2);
    static_assert(identifier_of(sps[1]) == "factor");
    static_assert(has_default_argument(sps[1]));
    static_assert(!has_default_argument(sps[0]));

    printf("04_params PASS\n");
}
