// Template machinery: substitute / can_substitute / template_of /
// template_arguments_of / reflections-as-NTTPs — the binder's spec-discovery
// and dispatch machinery, plus the TC-0004/0009 shape (same-named member
// templates dispatched through a reflection NTTP).
#include <meta>
#include <cstdio>
#include <string>

namespace api {
template <typename T> struct Box {
    T value;
    T get() const { return value; }
};
using IntBox = Box<int>;

struct Het {
    template <typename K = int> int lookup(K k) const { return int(k) + 1; }
    template <typename K = int> int lookup(K k)       { return int(k) + 2; }  // same head, non-const
};
} // namespace api

using namespace std::meta;

// reflection as NTTP + splice-call dispatch (binder's reflect_bind_method shape)
template <std::meta::info Fn>
int call_through(auto&& self, int arg) {
    return (self.[:Fn:])(arg);
}

int main() {
    static_assert(has_template_arguments(^^api::Box<int>));
    static_assert(template_of(^^api::Box<int>) == ^^api::Box);
    static_assert(is_template(^^api::Box));
    constexpr auto args = define_static_array(template_arguments_of(^^api::Box<int>));
    static_assert(args.size() == 1 && is_same_type(args[0], ^^int));

    // substitute + can_substitute
    static_assert(can_substitute(^^api::Box, {^^double}));
    constexpr info bd = substitute(^^api::Box, {^^double});
    static_assert(is_same_type(bd, ^^api::Box<double>));

    // member function template default-instantiation via substitute, then
    // dispatch through a reflection NTTP — two same-headed siblings must get
    // DISTINCT instantiations (the TC-0004/TC-0009 failure shape under clang).
    constexpr auto lookups = define_static_array([] consteval {
        std::vector<info> v;
        for (info m : members_of(^^api::Het, access_context::unchecked()))
            if (is_function_template(m)) v.push_back(m);
        return v;
    }());
    static_assert(lookups.size() == 2);
    constexpr info l0 = substitute(lookups[0], {^^int});
    constexpr info l1 = substitute(lookups[1], {^^int});
    api::Het h;
    const api::Het& ch = h;
    int a = call_through<l0>(ch, 10);
    int b = call_through<l1>(h, 10);
    if (a + b == 23 && a != b) printf("05_templates PASS\n");
    else { printf("05_templates FAIL a=%d b=%d\n", a, b); return 1; }
}
