// TC-0010 — reflections of a RE-OPENED namespace compare unequal across
// redeclarations: parent_of(^^member) != ^^ns whenever the member was declared
// in a block other than the namespace's first.
// Standalone repro (no nanobind). See
// corpus/findings/TC-0010-reopened-namespace-reflection-equality.md.
//
// Compile:
//   clang++ -std=c++26 -freflection-latest -fsyntax-only repro.cpp
//
// P2996 [expr.eq]: two reflections compare equal if they designate the same
// entity. A namespace is one entity no matter how many times it is re-opened,
// but reflection equality is implemented via APValue profiling
// (profileReflection, clang/lib/AST/APValue.cpp), which for
// ReflectionKind::Namespace profiles the RAW declaration pointer. ^^ns wraps
// the namespace's first NamespaceDecl; parent_of(^^member) wraps the
// NamespaceDecl of the block that declared the member. With one block they
// coincide (q1/q3 below always passed); with a re-opened namespace they are
// different redeclarations and the reflections compare UNEQUAL.
// ReflectionKind::Template in the same switch already canonicalizes
// (getCanonicalDecl) -- Namespace was the gap.
//
// Expected: compiles (all static_asserts hold).
// Actual at bloomberg/clang-p2996 @ 837da39eb88c (and the corpus base): the
// q2 static_assert FAILS -- "static assertion failed" on the re-opened
// namespace, no diagnostic of anything wrong.
//
// Field shape: Eigen re-opens Eigen::internal in nearly every header, so an
// enclosing-namespace test of the form
//     parent_of(template_of(^^Eigen::internal::pointer_based_stl_iterator<M>))
//         == ^^Eigen::internal
// silently answers FALSE -- the binder's nb::exclude_<^^Eigen::internal>
// namespace exclusion never matched anything declared in a later block, and
// internal iterator types leaked into the bind set.

#include <experimental/meta>

namespace a {
namespace b {
template <class T> struct S {};
}  // namespace b
}  // namespace a

namespace c {
namespace d {}
}  // namespace c
namespace c {
namespace d {                      // re-opened: R comes from the SECOND block
template <class T> struct R {};
}  // namespace d
}  // namespace c

// q1: single-block namespace -- always worked.
static_assert(std::meta::parent_of(^^a::b::S) == ^^a::b);

// q2: member declared in a re-opened block -- FAILS before the fix.
static_assert(std::meta::parent_of(^^c::d::R) == ^^c::d);

// q3: the same chain through a specialization's template -- follows q2.
static_assert(std::meta::parent_of(std::meta::template_of(^^c::d::R<int>)) ==
              ^^c::d);

int main() { return 0; }
