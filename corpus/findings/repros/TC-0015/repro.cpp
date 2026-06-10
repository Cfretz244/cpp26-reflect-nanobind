// TC-0015: a deduction-guide SPECIALIZATION reflection (Declaration kind, via
// substitute on the guide template) used as an NTTP ICEs the Itanium mangler:
// mangleReflection -> mangle -> mangleFunctionEncoding -> mangleUnqualifiedName
// -> llvm_unreachable("Can't mangle a deduction guide name!").
#include <experimental/meta>

namespace demo {
template <class T> struct Box { Box(T); };
Box(int) -> Box<int>;                       // explicit guide
}

template <std::meta::info R> struct Holder { static constexpr int x = 1; };

consteval bool is_guide(std::meta::info m) {
    return std::meta::is_function_template(m)
        && !std::meta::has_identifier(m)
        && !std::meta::is_operator_function_template(m)
        && !std::meta::is_conversion_function_template(m)
        && !std::meta::is_literal_operator_template(m)
        && !std::meta::is_constructor_template(m);
}

consteval std::meta::info guide_spec() {
    for (auto m : std::meta::members_of(^^demo,
                                        std::meta::access_context::unchecked()))
        if (is_guide(m) && std::meta::can_substitute(m, {^^int}))
            return std::meta::substitute(m, {^^int});
    return ^^void;
}

int use = Holder<guide_spec()>::x;   // mangling the NTTP mangles the reflection
