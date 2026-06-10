// TC-0012 — is_complete_type answers FALSE for a not-yet-instantiated (but
// perfectly instantiable) class-template specialization named through a type
// ALIAS: findTypeDecl on the TypedefType sugar returns the alias declaration,
// for which EnsureInstantiated is a no-op, so the spec is never instantiated
// and reported incomplete.
// Standalone repro (no nanobind). See
// corpus/findings/TC-0012-is-complete-type-alias-sugar.md.
//
// Compile:
//   clang++ -std=c++26 -freflection-latest -fsyntax-only repro.cpp
//
// is_complete_type (clang/lib/AST/ExprConstantMeta.cpp) does:
//     if (Decl *typeDecl = findTypeDecl(RV.getReflectedType()))
//       (void) Meta.EnsureInstantiated(typeDecl, Range);
//     result = !RV.getReflectedType()->isIncompleteType();
// without desugaring first -- unlike the members_of family, which desugars
// aliases before findTypeDecl. On a TypedefType, findTypeDecl returns the
// TypedefDecl; EnsureInstantiated(TypedefDecl) instantiates nothing, and the
// (never-yet-referenced) specialization then reads as incomplete. Naming the
// SAME spec directly first (q3) instantiates it and flips the alias answer to
// true -- order-dependent wrong answers.
//
// Expected: compiles (all static_asserts hold).
// Actual at bloomberg/clang-p2996 @ 837da39eb88c (and the corpus base): q1
// FAILS (alias-named spec reported incomplete).
//
// Field shape: spdlog's seeds are aliases (stdout_sink_mt =
// stdout_sink<console_mutex>) and its bind-set types reach signatures through
// member typedefs; a completeness-gated discovery walk (the binder's
// BINDER-0014 gate) silently dropped them.

#include <experimental/meta>

template <class T> struct Box { T v; };

using BoxInt = Box<int>;            // alias to a never-yet-instantiated spec
using BoxLong = Box<long>;

// q1: through the alias -- EnsureInstantiated must reach Box<int>.
static_assert(std::meta::is_complete_type(^^BoxInt));

// q2: a forward-declared-only template stays incomplete either way.
template <class T> struct Undefined;
using UndefinedInt = Undefined<int>;
static_assert(!std::meta::is_complete_type(^^UndefinedInt));
static_assert(!std::meta::is_complete_type(^^Undefined<long>));

// q3: the direct spelling instantiates; asking through the alias AFTERWARD
// also answers true (this passed even before the fix -- the bug is the
// order-dependence, q1 being the shape discovery walks actually hit).
static_assert(std::meta::is_complete_type(^^Box<long>));
static_assert(std::meta::is_complete_type(^^BoxLong));

int main() { return 0; }
