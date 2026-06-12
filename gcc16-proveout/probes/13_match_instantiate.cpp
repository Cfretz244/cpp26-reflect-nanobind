// Functional validation of nb::match_ and nb::instantiate_ at the
// seed-expansion / bind-set level (consteval asserts; compile with -c).

#include <nanobind/nb_reflect.h>

namespace nb = nanobind;
namespace nbd = nanobind::detail;

namespace mins {
struct VecA { int x; };
struct VecB { int y; };
struct Scalar {};
enum class Mode { fast, slow };
int vec_count();
int other_fn();
namespace impl { struct VecHidden {}; }      // rejected by the matcher below
namespace mathy { struct Inner {}; struct Other {}; }  // whole-ns match

template <class T, int N> struct Grid {
    T at(int) const;
    static constexpr int size = N;
};
template <class T> struct NeedsDefault { T value; };
template <class T> struct VecLike { T v; };
template <class T> struct OtherTmpl { T v; };
// a corner that fails SUBSTITUTION (constraint): product_ skips it silently.
// (A corner that substitutes but cannot COMPLETE -- e.g. a `T& ref` member
// with T=void -- is a hard error at the completeness gate, exactly as if the
// spec had been listed explicitly; constraints are how a grid declares its
// valid corners.)
template <class T> requires (!std::is_void_v<T>) struct NoVoid { T v; };
}  // namespace mins

using nbd::seeds_of;
using nbd::compute_bind_set;
using nbd::info_vec_contains;

consteval bool contains(const std::vector<std::meta::info>& v, std::meta::info x) {
    return info_vec_contains(v, x);
}

// --- plain elements expand to themselves; config markers to nothing ---
static_assert(seeds_of<^^mins>().size() == 1
              && seeds_of<^^mins>()[0] == ^^mins);
static_assert(seeds_of<^^mins::VecA>()[0] == (^^mins::VecA));
static_assert(seeds_of<^^nb::exclude_<^^mins::Scalar>>().empty());
static_assert(seeds_of<^^nb::exclude_if_<nb::named_<"x">>>().empty());
static_assert(seeds_of<^^nb::trampoline_all_>().empty());

// --- match_: named classes + functions, recursion, whole-namespace match ---
consteval bool match_basics() {
    auto s = seeds_of<^^nb::match_<^^mins, nb::named_<"Vec*">>>();
    return contains(s, ^^mins::VecA) && contains(s, ^^mins::VecB)
        && !contains(s, ^^mins::Scalar) && !contains(s, ^^mins::Mode)
        && !contains(s, ^^mins::VecLike)        // templates never match-seed
        && contains(s, ^^mins::impl::VecHidden) // recursed into impl
        && !contains(s, ^^mins::vec_count);     // fn doesn't match "Vec*"
}
static_assert(match_basics());

consteval bool match_functions_and_enums() {
    auto fns = seeds_of<^^nb::match_<^^mins, nb::is_function_>>();
    auto ens = seeds_of<^^nb::match_<^^mins, nb::is_enum_>>();
    return contains(fns, ^^mins::vec_count) && contains(fns, ^^mins::other_fn)
        && !contains(fns, ^^mins::VecA)
        && ens.size() == 1 && ens[0] == (^^mins::Mode);
}
static_assert(match_functions_and_enums());

// a nested namespace the matcher accepts is seeded WHOLE
consteval bool match_whole_namespace() {
    auto s = seeds_of<^^nb::match_<^^mins, nb::named_<"mathy">>>();
    return s.size() == 1 && s[0] == (^^mins::mathy);
}
static_assert(match_whole_namespace());

// exclusion gates the match_ walk (pack-wide exclude_if_ reaches it)
consteval bool match_respects_exclusion() {
    auto s = seeds_of<^^nb::match_<^^mins, nb::named_<"Vec*">>,
                      ^^nb::exclude_if_<nb::in_namespace_<^^mins::impl>>>();
    return contains(s, ^^mins::VecA) && !contains(s, ^^mins::impl::VecHidden);
}
static_assert(match_respects_exclusion());

// --- instantiate_: explicit template target, with_ + product_, val_ ---
consteval bool instantiate_with() {
    auto s = seeds_of<^^nb::instantiate_<^^mins::Grid,
                                         nb::with_<^^int, nb::val_<2>>>>();
    return s.size() == 1 && s[0] == (^^mins::Grid<int, 2>);
}
static_assert(instantiate_with());

consteval bool instantiate_product() {
    auto s = seeds_of<^^nb::instantiate_<^^mins::Grid,
                 nb::product_<nb::set_<^^float, ^^double>,
                              nb::set_<nb::val_<3>, nb::val_<4>>>>>();
    return s.size() == 4
        && contains(s, ^^mins::Grid<float, 3>)
        && contains(s, ^^mins::Grid<float, 4>)
        && contains(s, ^^mins::Grid<double, 3>)
        && contains(s, ^^mins::Grid<double, 4>);
}
static_assert(instantiate_product());

// a product_ corner that cannot substitute is silently skipped
consteval bool product_invalid_corner_skipped() {
    auto s = seeds_of<^^nb::instantiate_<^^mins::NoVoid,
                 nb::product_<nb::set_<^^int, ^^void>>>>();
    return s.size() == 1 && s[0] == (^^mins::NoVoid<int>);
}
static_assert(product_invalid_corner_skipped());

// empty with_ instantiates an all-defaulted... NeedsDefault<T> has no default;
// use with_ carrying one arg through an alias-reached marker instead, and a
// matcher-target rule: every class template named Vec* across the pack's
// namespace roots.
consteval bool instantiate_matcher_target() {
    auto s = seeds_of<^^nb::instantiate_<^^nb::named_<"Vec*">,
                                         nb::with_<^^int>>,
                      ^^mins>();
    return s.size() == 1 && s[0] == (^^mins::VecLike<int>);
}
static_assert(instantiate_matcher_target());

// matcher-target sweeps match_ scopes as namespace roots, too
consteval bool instantiate_matcher_target_via_match_scope() {
    auto s = seeds_of<^^nb::instantiate_<^^nb::named_<"Other*">,
                                         nb::with_<^^double>>,
                      ^^nb::match_<^^mins, nb::is_enum_>>();
    return s.size() == 1 && s[0] == (^^mins::OtherTmpl<double>);
}
static_assert(instantiate_matcher_target_via_match_scope());

// --- the bind set sees expanded seeds like explicit listings ---
consteval bool bind_set_integration() {
    auto set = compute_bind_set<
        ^^nb::match_<^^mins, nb::named_<"Vec?">>,
        ^^nb::instantiate_<^^mins::Grid,
            nb::product_<nb::set_<^^float>, nb::set_<nb::val_<3>>>>>();
    return contains(set, ^^mins::VecA) && contains(set, ^^mins::VecB)
        && contains(set, ^^mins::Grid<float, 3>)
        && !contains(set, ^^mins::Scalar);
}
static_assert(bind_set_integration());

// --- Python naming for minted specs rides the existing CamelCase path ---
consteval bool spec_names() {
    std::string_view n{nbd::entity_name<^^mins::Grid<float, 3>>()};
    return n == "GridFloat3";
}
static_assert(spec_names());
