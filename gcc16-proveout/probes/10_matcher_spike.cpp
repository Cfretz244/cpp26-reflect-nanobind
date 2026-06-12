// Spike for the matcher / default-instantiation API (umbrella plan step 0).
// Three mechanisms the design leans on, each of which must work on GCC 16:
//
//  (a) pointers to consteval functions (&matcher_invoke<M>) stored in a
//      consteval std::vector, passed across consteval functions, and called —
//      entirely inside constant evaluation, never persisted. This is the
//      exclude_if_ "exclusion_set { listed, preds }" mechanism.
//  (b) the matcher concept: an empty default-constructible type whose
//      consteval operator() is checked via a requires-expression (unevaluated,
//      so legal outside immediate function context), both at namespace scope
//      and from inside a consteval function. Both static and non-static
//      call-operator shapes.
//  (c) the marker round-trip: a matcher type carrying a fixed_string NTTP,
//      wrapped in match_<Scope, M>, passed as ^^marker through an info-NTTP
//      function template, recovered via template_arguments_of + splice, and
//      invoked. This is how seeds_of<R>() extracts the matcher.
//
// Expected: compiles + runs with exit code 0.

#include <meta>
#include <vector>
#include <concepts>
#include <type_traits>

// ---- shared fixture -------------------------------------------------------

namespace fix {
struct VecA {};
struct VecB {};
struct Scalar {};
inline namespace detail_probe {}
}  // namespace fix

// ---- (b) the concept ------------------------------------------------------

template <typename M>
concept matcher = std::is_empty_v<M> && std::is_default_constructible_v<M> &&
    requires(std::meta::info r) {
        { M{}(r) } -> std::same_as<bool>;
    };

template <unsigned N> struct fixed_string {
    char data[N] = {};
    consteval fixed_string(const char (&s)[N]) {
        for (unsigned i = 0; i < N; ++i) data[i] = s[i];
    }
};

consteval bool glob_match(const char* pat, const char* s) {
    // two-pointer, * and ? only
    const char *star = nullptr, *ss = nullptr;
    while (*s) {
        if (*pat == '?' || *pat == *s) { ++pat; ++s; }
        else if (*pat == '*') { star = pat++; ss = s; }
        else if (star) { pat = star + 1; s = ++ss; }
        else return false;
    }
    while (*pat == '*') ++pat;
    return *pat == '\0';
}

// non-static consteval call operator
template <fixed_string Pat> struct named_ {
    consteval bool operator()(std::meta::info r) const {
        if (!std::meta::is_type(r)) return false;
        auto d = std::meta::dealias(r);
        if (std::meta::has_template_arguments(d)) d = std::meta::template_of(d);
        if (!std::meta::has_identifier(d)) return false;
        return glob_match(Pat.data, std::meta::identifier_of(d).data());
    }
};

// static consteval call operator (C++23 static operator())
struct is_class_ {
    static consteval bool operator()(std::meta::info r) {
        return std::meta::is_type(r) &&
               std::meta::is_class_type(std::meta::dealias(r));
    }
};

static_assert(matcher<named_<"Vec*">>);   // namespace-scope concept check
static_assert(matcher<is_class_>);
static_assert(!matcher<int>);

consteval bool concept_from_immediate_context() {
    return matcher<named_<"x">> && matcher<is_class_> && !matcher<double>;
}
static_assert(concept_from_immediate_context());

// ---- (a) consteval function pointers in consteval vectors ------------------

using matcher_fn = bool (*)(std::meta::info);

template <typename M>
consteval bool matcher_invoke(std::meta::info r) { return M{}(r); }

consteval std::vector<matcher_fn> build_preds() {
    std::vector<matcher_fn> preds;
    preds.push_back(&matcher_invoke<named_<"Vec*">>);
    preds.push_back(&matcher_invoke<is_class_>);
    return preds;
}

consteval bool any_pred_matches(const std::vector<matcher_fn>& preds,
                                std::meta::info r) {
    for (auto p : preds)
        if (p(r)) return true;
    return false;
}

consteval int count_matches(std::meta::info ns) {
    auto preds = build_preds();           // built in one consteval fn...
    int n = 0;
    for (auto m : std::meta::members_of(ns, std::meta::access_context::unchecked()))
        if (std::meta::is_type(m) && any_pred_matches(preds, m))  // ...called in another
            ++n;
    return n;
}
static_assert(count_matches(^^fix) == 3);  // VecA, VecB (named_), Scalar (is_class_)

// glob engine truth table while we're here
static_assert(glob_match("Vec*", "VecA"));
static_assert(glob_match("*View", "SparseView"));
static_assert(glob_match("?at", "mat"));
static_assert(!glob_match("Vec*", "Scalar"));
static_assert(glob_match("*", ""));
static_assert(!glob_match("?", ""));

// ---- (c) marker round-trip --------------------------------------------------

template <std::meta::info Scope, typename M> struct match_ {};

template <std::meta::info Marker>
consteval std::vector<std::meta::info> seeds_of_match() {
    constexpr auto args = std::define_static_array(
        std::meta::template_arguments_of(std::meta::dealias(Marker)));
    // an info NTTP arrives as a reflection OF the value: extract it
    constexpr auto scope = std::meta::extract<std::meta::info>(args[0]);
    constexpr auto mty   = args[1];          // a type reflection of the matcher
    using M = [:mty:];
    static_assert(matcher<M>);
    std::vector<std::meta::info> out;
    for (auto m : std::meta::members_of(scope, std::meta::access_context::unchecked()))
        if (std::meta::is_type(m) && M{}(m))
            out.push_back(m);
    return out;
}

consteval bool marker_roundtrip() {
    constexpr auto marker = ^^match_<^^fix, named_<"Vec?">>;
    auto seeds = seeds_of_match<marker>();
    return seeds.size() == 2 &&
           seeds[0] == (^^fix::VecA) && seeds[1] == (^^fix::VecB);
}
static_assert(marker_roundtrip());

// (c2) the same extraction shape exclude_if_ needs: marker type -> matcher_fn
template <typename M> struct exclude_if_ {};

template <std::meta::info Marker>
consteval matcher_fn matcher_of_exclude_if() {
    constexpr auto margs = std::define_static_array(
        std::meta::template_arguments_of(std::meta::dealias(Marker)));
    return &matcher_invoke<typename [:margs[0]:]>;
}

consteval bool exclude_if_roundtrip() {
    std::vector<matcher_fn> preds;
    preds.push_back(matcher_of_exclude_if<^^exclude_if_<named_<"Sca*">>>());
    return any_pred_matches(preds, ^^fix::Scalar) &&
           !any_pred_matches(preds, ^^fix::VecA);
}
static_assert(exclude_if_roundtrip());

int main() { return 0; }
