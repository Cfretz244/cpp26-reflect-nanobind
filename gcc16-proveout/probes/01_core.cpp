// Core P2996 queries the binder leans on hardest.
#include <meta>
#include <cstdio>
#include <ranges>
#include <vector>

namespace demo {
struct Point { int x; int y; double mag() const noexcept; };
enum class Color { Red, Green };
using Alias = Point;
namespace inner { struct Hidden {}; }
} // namespace demo

using namespace std::meta;

consteval int count_public_methods(info cls) {
    int n = 0;
    for (info m : members_of(cls, access_context::unchecked()))
        if (is_function(m) && is_public(m) && !is_constructor(m) && !is_destructor(m)
            && !is_operator_function(m) && !is_special_member_function(m))
            ++n;
    return n;
}

int main() {
    // identity / naming / sugar
    static_assert(identifier_of(^^demo::Point) == "Point");
    static_assert(has_identifier(^^demo::Point));
    static_assert(dealias(^^demo::Alias) == ^^demo::Point);
    static_assert(is_type_alias(^^demo::Alias));
    static_assert(parent_of(^^demo::Point) == ^^demo);
    static_assert(is_namespace(^^demo));
    static_assert(is_class_type(^^demo::Point));
    static_assert(is_enum_type(^^demo::Color));
    static_assert(is_complete_type(^^demo::Point));

    // member enumeration + type splice
    constexpr auto fields = define_static_array(
        nonstatic_data_members_of(^^demo::Point, access_context::unchecked()));
    static_assert(fields.size() == 2);
    static_assert(identifier_of(fields[0]) == "x");
    static_assert(is_same_type(type_of(fields[0]), ^^int));
    typename [:type_of(fields[0]):] v = 42;  // splice as a type
    static_assert(count_public_methods(^^demo::Point) == 1);

    // function shape queries
    constexpr info magf = [] consteval {
        for (info m : members_of(^^demo::Point, access_context::unchecked()))
            if (is_function(m) && has_identifier(m) && identifier_of(m) == "mag")
                return m;
        return info{};
    }();
    static_assert(is_const(type_of(magf)));
    static_assert(is_noexcept(type_of(magf)));
    static_assert(is_same_type(return_type_of(magf), ^^double));
    static_assert(!is_static_member(magf));

    // enums
    constexpr auto ens = define_static_array(enumerators_of(^^demo::Color));
    static_assert(ens.size() == 2 && identifier_of(ens[1]) == "Green");

    // bases
    static_assert(bases_of(^^demo::Point, access_context::unchecked()).size() == 0);

    printf("01_core PASS (v=%d)\n", v);
}
