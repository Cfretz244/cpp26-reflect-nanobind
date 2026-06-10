// TC-0011 — members_of(^^ns) derails on a re-opened namespace block that
// STARTS with an out-of-line class-member definition: the definition PATTERN
// is enumerated as a namespace "member" and every other member of the
// namespace is silently dropped.
// Standalone repro (no nanobind). See
// corpus/findings/TC-0011-namespace-walk-out-of-line-member-defs.md.
//
// Compile and run:
//   clang++ -std=c++26 -freflection-latest repro.cpp && ./a.out
//
// members_of for a namespace iterates each NamespaceDecl block's lexical decl
// chain, hopping to the previous block when one is exhausted
// (findIterableMember, clang/lib/AST/ExprConstantMeta.cpp). Two defects
// compound when a re-opened block BEGINS with an out-of-line member
// definition (lexically in the namespace, semantically in the class):
//   1. isReflectableDecl's redeclaration filter has a hole: the out-of-line
//      definition is the FIRST declaration in its own lexical context, so the
//      "is this the first redeclaration here" check passes and the definition
//      (for a class template: the dependent definition PATTERN) is yielded as
//      a namespace member. Reflections of such a pattern break downstream
//      metafunctions (display_string_of / use as a template argument hit
//      "isa<> used on a null pointer" for dependent signatures).
//   2. Stepping FROM that decl consults its SEMANTIC DeclContext (the class)
//      and walks the class's multi-context chain instead of the namespace's
//      lexical chain -- terminating the walk and silently dropping every
//      remaining namespace member.
//
// Expected: members_of(^^n) yields Tmpl, h, k (the definition of Tmpl<T>::g
// is a redeclaration of a CLASS member, not a namespace member).
// Actual at bloomberg/clang-p2996 @ 837da39eb88c (and the corpus base): the
// walk yields exactly ONE "member" -- g, the out-of-line definition pattern.
//
// Field shape: Eigen defines most facade members out-of-line in re-opened
// `namespace Eigen` blocks (Eigen/src/Geometry/EulerAngles.h begins with the
// MatrixBase<Derived>::canonicalEulerAngles definition), so ANY namespace-
// member lift over ^^Eigen (the binder's free-operator scan) either crashed
// the compiler (mangling/display of the dependent pattern) or silently saw a
// gutted namespace.

#include <experimental/meta>
#include <cstdio>
#include <string_view>
#include <vector>

namespace n {
template <class T> struct Tmpl { int g() const; };
inline int h() { return 3; }
}

namespace n {                                 // re-opened; FIRST decl is the
template <class T> int Tmpl<T>::g() const {   // out-of-line definition PATTERN
    return 2;
}
inline int k() { return 4; }
}

consteval bool has_member(std::string_view name) {
    for (auto m : std::meta::members_of(^^n,
                                        std::meta::access_context::unchecked()))
        if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name)
            return true;
    return false;
}

int main() {
    static_assert(has_member("Tmpl"));   // dropped before the fix
    static_assert(has_member("h"));      // dropped before the fix
    static_assert(has_member("k"));      // dropped before the fix
    static_assert(!has_member("g"));     // the definition pattern, wrongly
                                         // yielded before the fix
    std::puts("ok");
    return 0;
}
