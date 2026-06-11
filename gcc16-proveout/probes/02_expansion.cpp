// Expansion statements (P1306): `template for` over define_static_array,
// including the nested class-then-members shape the binder uses everywhere.
#include <meta>
#include <cstdio>
#include <string>

namespace api {
struct A { int i; float f; };
struct B { double d; };
} // namespace api

using namespace std::meta;

consteval auto classes_in(info ns) {
    std::vector<info> out;
    for (info m : members_of(ns, access_context::unchecked()))
        if (is_class_type(m)) out.push_back(m);
    return out;
}

int main() {
    int total = 0;
    template for (constexpr auto cls : define_static_array(classes_in(^^api))) {
        printf("class %s\n", std::string(identifier_of(cls)).c_str());
        template for (constexpr auto mem : define_static_array(
                          nonstatic_data_members_of(cls, access_context::unchecked()))) {
            // splice the member inside the nested loop body — the binder's core move
            using T = typename [:type_of(mem):];
            printf("  .%s (size %zu)\n", std::string(identifier_of(mem)).c_str(), sizeof(T));
            ++total;
        }
    }
    if (total != 3) { printf("02_expansion FAIL total=%d\n", total); return 1; }
    printf("02_expansion PASS\n");
}
