// Annotations (P3394): [[=value]] with struct values carrying fixed strings,
// queried via annotations_of — the binder's skip/rename/doc mechanism.
#include <meta>
#include <cstdio>
#include <algorithm>

template <size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
};
template <size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

namespace ann {
struct skip_t {};
inline constexpr skip_t skip{};
template <size_t N> struct rename { fixed_string<N> name; };
template <size_t N> rename(fixed_string<N>) -> rename<N>;
} // namespace ann

namespace api {
struct Widget {
    [[=ann::skip]] int internal_state;
    [[=ann::rename{fixed_string{"size"}}]] int sz;
    int plain;
};
} // namespace api

using namespace std::meta;

consteval bool has_skip(info mem) {
    for (info a : annotations_of(mem))
        if (is_same_type(remove_cvref(type_of(a)), ^^ann::skip_t)) return true;
    return false;
}

int main() {
    constexpr auto mems = define_static_array(
        nonstatic_data_members_of(^^api::Widget, access_context::unchecked()));
    static_assert(has_skip(mems[0]));
    static_assert(!has_skip(mems[1]));
    static_assert(!has_skip(mems[2]));

    // extract the rename value back out (constant_of + extract round trip)
    constexpr auto anns = define_static_array(annotations_of(mems[1]));
    static_assert(anns.size() == 1);
    constexpr auto rn = extract<ann::rename<5>>(constant_of(anns[0]));
    static_assert(rn.name.data[0] == 's' && rn.name.data[3] == 'e');

    printf("03_annotations PASS\n");
}
