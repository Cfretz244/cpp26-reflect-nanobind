// Functional validation of nb::exclude_if_ at the bind-set/discovery level
// (no Python module build; consteval asserts only -- compile with
// -fsyntax-only is NOT enough since static_asserts evaluate at parse, so a
// plain compile of this TU suffices; it has no main and builds with -c).
//
// Checks: predicate exclusion drops namespace seeds, applies transitively to
// namespace contents (parent-chain congruence with listed entries), gates
// spec discovery (signature mentions), works nested inside exclude_<...>,
// and an empty exclusion still behaves.

#include <nanobind/nb_reflect.h>

namespace nb = nanobind;
namespace nbd = nanobind::detail;

namespace xifns {
struct Keep {};
struct DropMe {};
namespace internal {           // excluded via in_namespace_ matcher
struct Hidden {};
struct AlsoHidden { struct Nested {}; };
}
template <class T> struct Wrap { T value; };
struct Api {
    Wrap<Keep> good() const;            // discovers Wrap<Keep>
    Wrap<DropMe> bad() const;           // signature mentions excluded -> spec not discovered
    internal::Hidden leak() const;      // mentions excluded namespace content
};
}  // namespace xifns

using nbd::compute_bind_set;
using nbd::excluded_q;
using nbd::is_excluded_entity;
using nbd::required_user_specs;
using nbd::info_vec_contains;

// --- is_excluded_entity with predicates: entity / template / parent congruence
constexpr auto PACK_NS  = ^^xifns;
constexpr auto X_DROP   = ^^nb::exclude_if_<nb::named_<"Drop*">>;
constexpr auto X_INTERN = ^^nb::exclude_if_<nb::in_namespace_<^^xifns::internal>>;
// the same predicate nested inside exclude_<...>:
constexpr auto X_NESTED = ^^nb::exclude_<^^nb::exclude_if_<nb::named_<"Drop*">>>;

consteval bool direct_hits() {
    auto ex = excluded_q<PACK_NS, X_DROP, X_INTERN>();
    return is_excluded_entity(^^xifns::DropMe, ex)
        && is_excluded_entity(^^xifns::internal::Hidden, ex)
        && is_excluded_entity(^^xifns::internal::AlsoHidden::Nested, ex)
        && !is_excluded_entity(^^xifns::Keep, ex)
        && !is_excluded_entity(^^xifns::Api, ex);
}
static_assert(direct_hits());

consteval bool nested_placement() {
    auto ex = excluded_q<PACK_NS, X_NESTED>();
    return is_excluded_entity(^^xifns::DropMe, ex)
        && !is_excluded_entity(^^xifns::Keep, ex);
}
static_assert(nested_placement());

// a predicate matching a NAMESPACE excludes its contents transitively,
// exactly like listing the namespace:
consteval bool namespace_name_pred() {
    auto ex = excluded_q<PACK_NS, ^^nb::exclude_if_<nb::named_<"internal">>>();
    return is_excluded_entity(^^xifns::internal::Hidden, ex)
        && !is_excluded_entity(^^xifns::Keep, ex);
}
static_assert(namespace_name_pred());

// --- bind set: seeds dropped, spec discovery gated
consteval bool bind_set_shape() {
    auto set = compute_bind_set<PACK_NS, X_DROP, X_INTERN>();
    return info_vec_contains(set, ^^xifns::Keep)
        && info_vec_contains(set, ^^xifns::Api)
        && info_vec_contains(set, ^^xifns::Wrap<xifns::Keep>)     // discovered
        && !info_vec_contains(set, ^^xifns::DropMe)               // pred-dropped seed
        && !info_vec_contains(set, ^^xifns::Wrap<xifns::DropMe>)  // tainted signature
        && !info_vec_contains(set, ^^xifns::internal::Hidden);
}
static_assert(bind_set_shape());

// --- without markers, nothing changes
consteval bool no_marker_baseline() {
    auto set = compute_bind_set<PACK_NS>();
    return info_vec_contains(set, ^^xifns::DropMe)
        && info_vec_contains(set, ^^xifns::internal::Hidden)
        && info_vec_contains(set, ^^xifns::Wrap<xifns::DropMe>);
}
static_assert(no_marker_baseline());
