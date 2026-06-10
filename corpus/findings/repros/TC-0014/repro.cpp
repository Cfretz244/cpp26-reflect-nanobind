// TC-0014 minimal: a metafunction error path DESCRIBING a builtin template
// crashed instead of diagnosing. -fsyntax-only suffices.
//   return_type_of(^^__make_integer_seq-found-via-members_of(^^::))
// At base: UNREACHABLE "unhandled template kind" (ExprConstantMeta.cpp,
// DescriptionOf). Fixed: clean "cannot query the return type of a builtin
// template" diagnostic. Field shape: reflecting ANY class in the GLOBAL
// namespace -- box2d's entire API -- walked into one via the binder's
// free-operator scan (recovered diagnostic path), ICEing every box2d TU.
#include <experimental/meta>
consteval std::meta::info find_builtin_template() {
    for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
        if (std::meta::is_template(m) && std::meta::has_identifier(m)
            && std::meta::identifier_of(m) == "__make_integer_seq")
            return m;
    return ^^void;
}
constexpr auto r = std::meta::return_type_of(find_builtin_template());
