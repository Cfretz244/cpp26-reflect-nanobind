// Standalone validation of nanobind/nb_reflect_match.h (matcher DSL):
// truth tables for the glob engine, every leaf (incl. hostile-input guards),
// and the combinators. Compiles with only the header + <meta>.
// Expected: compiles + runs with exit code 0.

#include <nanobind/nb_reflect_match.h>

namespace nb = nanobind;

namespace fixns {
struct VecA {};
struct VecB { struct Nested {}; };
struct Scalar {};
enum class Color { red };
inline int counter = 0;
void free_fn();
template <class T> struct Box {};
using BoxInt = Box<int>;
namespace inner {
struct Deep {};
namespace deeper { struct Deepest {}; }
}  // namespace inner
struct ForwardOnly;  // never defined
struct BaseC {};
struct MidC : BaseC {};
struct DerC : MidC {};
struct PrivDer : private BaseC {};
}  // namespace fixns
// re-opened block: in_namespace_ must see members from later declarations
namespace fixns {
struct Reopened {};
}

// --- glob engine ---
using nb::detail::glob_match;
static_assert(glob_match("Vec*", "VecA"));
static_assert(glob_match("Vec*", "Vec"));
static_assert(!glob_match("Vec*", "Scalar"));
static_assert(glob_match("*View", "SparseView"));
static_assert(glob_match("*View", "View"));
static_assert(!glob_match("*View", "Viewer"));
static_assert(glob_match("?at", "mat"));
static_assert(!glob_match("?at", "at"));
static_assert(glob_match("a*b*c", "aXXbYYc"));
static_assert(!glob_match("a*b*c", "aXXbYY"));
static_assert(glob_match("*", ""));
static_assert(glob_match("", ""));
static_assert(!glob_match("", "x"));
static_assert(glob_match("exact", "exact"));

// --- concept ---
static_assert(nb::matcher<nb::is_class_>);
static_assert(nb::matcher<nb::named_<"Vec*">>);
static_assert(nb::matcher<nb::all_of_<nb::is_class_, nb::named_<"V*">>>);
static_assert(!nb::matcher<int>);

// --- leaves: positives ---
static_assert(nb::is_class_{}(^^fixns::VecA));
static_assert(nb::is_class_{}(^^fixns::BoxInt));            // through alias
static_assert(nb::is_enum_{}(^^fixns::Color));
static_assert(nb::is_function_{}(^^fixns::free_fn));
static_assert(nb::is_template_{}(^^fixns::Box));
static_assert(nb::named_<"Vec?">{}(^^fixns::VecA));
static_assert(nb::named_<"Box">{}(^^fixns::Box<int>));      // spec -> template name
static_assert(nb::named_<"Box">{}(^^fixns::BoxInt));        // alias of spec, too
static_assert(nb::named_<"free*">{}(^^fixns::free_fn));
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::VecA));
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::inner::Deep));        // transitive
static_assert(nb::in_namespace_<^^fixns::inner>{}(^^fixns::inner::deeper::Deepest));
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::Reopened));           // re-opened block
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::free_fn));
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::Box));
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::counter));
static_assert(nb::derived_from_<^^fixns::BaseC>{}(^^fixns::BaseC));       // base itself
static_assert(nb::derived_from_<^^fixns::BaseC>{}(^^fixns::MidC));
static_assert(nb::derived_from_<^^fixns::BaseC>{}(^^fixns::DerC));        // transitive
static_assert(nb::derived_from_<^^fixns::BaseC>{}(^^fixns::PrivDer));     // private base

// --- leaves: negatives + hostile inputs (must answer false, never throw) ---
static_assert(!nb::is_class_{}(^^fixns::Color));
static_assert(!nb::is_class_{}(^^fixns));                   // namespace
static_assert(!nb::is_class_{}(^^int));
static_assert(!nb::is_class_{}(^^fixns::free_fn));
static_assert(!nb::is_enum_{}(^^fixns::VecA));
static_assert(!nb::is_function_{}(^^fixns::VecA));
static_assert(!nb::is_template_{}(^^fixns::Box<int>));      // a spec is not a template
static_assert(!nb::named_<"Vec*">{}(^^fixns::Scalar));
static_assert(!nb::named_<"Vec*">{}(^^int));                // builtin: no identifier
static_assert(!nb::named_<"x">{}(^^::));                    // global ns
static_assert(!nb::in_namespace_<^^fixns>{}(^^fixns));      // ns not inside itself
static_assert(!nb::in_namespace_<^^fixns::inner>{}(^^fixns::VecA));
static_assert(!nb::in_namespace_<^^fixns>{}(^^int));        // builtin
// the walk looks THROUGH class scopes (the exclude_ namespace-entry rule):
static_assert(nb::in_namespace_<^^fixns>{}(^^fixns::VecB::Nested));
static_assert(!nb::in_namespace_<^^fixns::inner>{}(^^fixns::VecB::Nested));
static_assert(!nb::derived_from_<^^fixns::BaseC>{}(^^fixns::Scalar));
static_assert(!nb::derived_from_<^^fixns::BaseC>{}(^^fixns::ForwardOnly)); // incomplete
static_assert(!nb::derived_from_<^^fixns::BaseC>{}(^^fixns));              // namespace
static_assert(!nb::derived_from_<^^fixns::BaseC>{}(^^int));

// --- combinators ---
static_assert(nb::all_of_<nb::is_class_, nb::named_<"Vec*">,
                          nb::in_namespace_<^^fixns>>{}(^^fixns::VecA));
static_assert(!nb::all_of_<nb::is_class_, nb::named_<"Vec*">>{}(^^fixns::Scalar));
static_assert(nb::any_of_<nb::named_<"Sca*">, nb::named_<"Vec*">>{}(^^fixns::Scalar));
static_assert(!nb::any_of_<nb::named_<"X">, nb::named_<"Y">>{}(^^fixns::Scalar));
static_assert(nb::not_<nb::is_enum_>{}(^^fixns::VecA));
static_assert(!nb::not_<nb::is_class_>{}(^^fixns::VecA));
static_assert(nb::all_of_<>{}(^^fixns::VecA));              // vacuous truth
static_assert(!nb::any_of_<>{}(^^fixns::VecA));

int main() { return 0; }
