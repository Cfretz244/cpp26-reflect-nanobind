// BINDER-0016 investigation: does a member-overload reflection survive the
// substitute(^^exclude_, {reflect_constant(info)}) round-trip with identity?
#include <experimental/meta>
namespace nb { template <std::meta::info... Rs> struct exclude_ {}; }

struct node_type {};
struct node {
    virtual bool is_homogeneous(node_type, node*&) noexcept { return false; }
    virtual bool is_homogeneous(node_type, const node*&) const noexcept { return false; }
    int len() const { return 1; }
};

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

consteval bool roundtrip_identity() {
    auto orig = bad_overloads();
    auto marker = make_marker();
    auto targs = std::meta::template_arguments_of(marker);
    if (targs.size() != orig.size()) return false;
    for (size_t i = 0; i < targs.size(); ++i)
        if (std::meta::extract<std::meta::info>(targs[i]) != orig[i])
            return false;
    return true;
}
static_assert(roundtrip_identity());
int main() {}
