// TC-0017: is_class_type over the predeclared global NEON typedefs
// (__Int64x1_t: element type is plain `long` on LP64 Darwin) instantiates a
// builtin-expansion specialization whose eager mangling hits
// llvm_unreachable("unexpected Neon vector element type") in
// mangleNeonVectorType. Compile with -c (codegen); -fsyntax-only is clean.
#include <experimental/meta>
#include <string_view>
consteval int probe() {
    int n = 0;
    for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked())) {
        if (!std::meta::has_identifier(m)) continue;
        if (std::string_view(std::meta::identifier_of(m)).starts_with("__Int64"))
            if (std::meta::is_type(m) && std::meta::is_class_type(m)) ++n;
    }
    return n;
}
int main() { static_assert(probe() >= 0); return 0; }
