// BINDER-0016: mimic the binder's actual gate -- excluded_v (define_static_array
// of extracted marker args) consulted against the bind loop's lifted members --
// with out-of-line definitions adding redeclarations.
#include <experimental/meta>
namespace nb { template <std::meta::info... Rs> struct exclude_ {}; }

struct node_type {};
struct node {
    virtual bool is_homogeneous(node_type, node*&) noexcept;
    virtual bool is_homogeneous(node_type, const node*&) const noexcept;
    int len() const { return 1; }
};
// out-of-line definitions => redeclarations exist
bool node::is_homogeneous(node_type, node*&) noexcept { return false; }
bool node::is_homogeneous(node_type, const node*&) const noexcept { return false; }

consteval std::vector<std::meta::info> bad_overloads() {
    std::vector<std::meta::info> out;
    for (auto m : std::meta::members_of(^^node, std::meta::access_context::unchecked()))
        if (std::meta::is_function(m) && std::meta::has_identifier(m)
            && std::meta::identifier_of(m) == "is_homogeneous")
            out.push_back(m);
    return out;
}
consteval std::meta::info make_marker() {
    std::vector<std::meta::info> args;
    for (auto m : bad_overloads())
        args.push_back(std::meta::reflect_constant(m));
    return std::meta::substitute(^^nb::exclude_, args);
}
consteval std::vector<std::meta::info> compute_excluded(std::meta::info marker) {
    std::vector<std::meta::info> out;
    for (auto a : std::meta::template_arguments_of(marker))
        out.push_back(std::meta::extract<std::meta::info>(a));
    return out;
}
inline constexpr auto excluded_arr =
    std::define_static_array(compute_excluded(make_marker()));

consteval bool span_contains(std::span<const std::meta::info> v, std::meta::info x) {
    for (auto e : v) if (e == x) return true;
    return false;
}
consteval bool gate_works() {
    int seen = 0;
    for (auto fn : std::define_static_array(std::meta::members_of(
             ^^node, std::meta::access_context::unchecked()))) {
        if (!std::meta::is_function(fn) || !std::meta::has_identifier(fn)) continue;
        if (std::meta::identifier_of(fn) == "is_homogeneous") {
            ++seen;
            if (!span_contains(excluded_arr, fn)) return false;
        }
        if (std::meta::identifier_of(fn) == "len"
            && span_contains(excluded_arr, fn)) return false;
    }
    return seen == 2;
}
static_assert(gate_works());
int main() {}
