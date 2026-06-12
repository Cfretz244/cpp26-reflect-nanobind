// GCC-8: same-named member-function-template reflections used as NTTPs
// mangle IDENTICALLY, so a dispatcher instantiated for two same-named
// sibling templates (a const/non-const overload pair) emits one assembly
// symbol twice: "Error: symbol `..._ZN...dispatchI...atE...' is already
// defined" at assembly time. The clang-p2996 fork had the same family of
// bugs (its TC-0004/TC-0009, fixed by folding an ODR hash of the template
// head + pattern type into the mangling); stock GCC 16.1 has the
// counterpart bug.
//
// Field shape: ankerl::unordered_dense's table::at(K)/at(K) const
// heterogeneous-lookup pair, dispatched through the binder's
// reflect_bind_member_template<T, tmpl>.
//
//   g++ -std=c++26 -freflection xfail_gcc8_member_template_nttp_mangling.cpp
//
// Expected: compiles, runs, exits 0 (clang-p2996 behavior after TC-0004/9).
// Actual (gcc 16.1.0): assembler error, symbol already defined.
#include <meta>
#include <string_view>
#include <vector>

struct Table {
    int v = 41;
    // Same-named member-template overload pair (const / non-const), both
    // default-instantiable -- the heterogeneous-lookup shape.
    template <class K = int>
    int& at(const K&) { return v; }
    template <class K = int>
    const int& at(const K&) const { return v; }
};

// The binder's dispatch shape: one function template instantiated once per
// sibling, with the sibling's TEMPLATE reflection as an NTTP. The two
// instantiations must get distinct symbols.
template <typename T, std::meta::info Tmpl>
void dispatch(T& obj, int& out) {
    constexpr auto spec =
        std::meta::substitute(Tmpl, std::vector<std::meta::info>{});
    constexpr auto mp = &[:spec:];
    out += (obj.*mp)(0);
}

int main() {
    Table t;
    int sum = 0;
    template for (constexpr auto m : std::define_static_array(
                      std::meta::members_of(^^Table,
                          std::meta::access_context::unchecked()))) {
        if constexpr (std::meta::is_function_template(m)
                      && std::meta::has_identifier(m)
                      && std::meta::identifier_of(m) ==
                             std::string_view("at")) {
            dispatch<Table, m>(t, sum);
        }
    };
    return sum == 82 ? 0 : 1;
}
