// The emit backend's pattern: heavy consteval std::string building lifted out
// via define_static_string / reflect_constant_string, display_string_of on
// types, symbol_of on operators. This is what Phase 4 (write_bindings) needs.
#include <meta>
#include <cstdio>
#include <string>

namespace api {
struct Thing {
    int n;
    bool operator==(const Thing&) const = default;
    Thing operator*(int k) const { return {n * k}; }
};
} // namespace api

using namespace std::meta;

consteval std::string render(info cls) {
    std::string out = "class ";
    out += identifier_of(cls);
    for (info m : members_of(cls, access_context::unchecked())) {
        if (is_nonstatic_data_member(m)) {
            out += " | field ";
            out += identifier_of(m);
            out += " : ";
            out += display_string_of(type_of(m));
        } else if (is_operator_function(m) && !is_special_member_function(m)) {
            out += " | ";
            out += symbol_of(operator_of(m));
        }
    }
    return out;
}

int main() {
    constexpr auto txt = define_static_string(render(^^api::Thing));
    printf("%s\n", txt);
    constexpr std::string_view sv{txt};
    static_assert(sv.find("field n : int") != sv.npos);
    // NOTE: clang-p2996's symbol_of returns "operator*"; check what GCC gives.
    static_assert(sv.find('*') != sv.npos);
    printf("07_consteval_strings PASS\n");
}
