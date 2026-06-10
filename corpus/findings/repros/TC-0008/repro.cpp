// TC-0008 — mangling a reflection of a DEDUCTION GUIDE (as a
// define_static_array element / reflection NTTP) ICEs the Itanium mangler:
//   Can't mangle a deduction guide name!
//   UNREACHABLE executed at clang/lib/AST/ItaniumMangle.cpp:1774
// Standalone repro (no nanobind, no library). See
// corpus/findings/TC-0008-deduction-guide-reflection-mangle-ice.md.
//
// members_of over a namespace enumerates deduction guides like any other
// member. Lifting the member list into static storage with
// define_static_array gives the backing FixedArray specialization a linkage
// name in which every element reflection is mangled as a template argument
// (mangleReflection). For a reflection of a template, the mangler encodes the
// template's NAME (mangleTemplateName -> mangleUnqualifiedName); a deduction
// guide's DeclarationName is CXXDeductionGuideNameKind, which
// mangleUnqualifiedName does not handle -> llvm_unreachable. Same component
// family as TC-0004 (mangleReflection / ReflectionKind::Template), different
// declaration-name kind.
//
// Build matrix (from the umbrella repo root):
//   TC=$PWD/toolchain
//   CXX="$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot $(xcrun --show-sdk-path) -nostdinc++ -isystem $TC/include/c++/v1"
//   $CXX -DGUIDE -c repro.cpp -o /tmp/r.o   # ICE (the bug; needs codegen)
//   $CXX -c repro.cpp -o /tmp/r.o           # clean (control: no guide)
//   $CXX -DGUIDE -fsyntax-only repro.cpp    # clean (front-end is fine; it is the mangler)
//
// Field shape: TartanLlama/expected v1.3.1 declares
//   template <class E> unexpected(E) -> unexpected<E>;
// at namespace scope (behind #ifdef __cpp_deduction_guides, so it is always
// present at C++26). The reflection-driven nanobind binder's
// bind_free_operators pass does
//   template for (constexpr auto fn : define_static_array(members_of(^^tl, ...)))
// while binding ANY tl class, so every tl::expected / tl::unexpected binding
// ICEs at codegen -- this is the recorded Gate-4 failure of
// corpus/runs/expected. A binder-side "skip deduction guides" filter would
// dodge it, but storing a deduction-guide reflection in mangled-name position
// should not crash the compiler regardless.
#include <experimental/meta>

namespace demo {
template <class E> struct unexpected { unexpected(E); };
#ifdef GUIDE
template <class E> unexpected(E) -> unexpected<E>;
#endif
}

void walk() {
  template for (constexpr auto m :
      std::define_static_array(std::meta::members_of(
          ^^demo, std::meta::access_context::unchecked()))) {
    constexpr bool t = std::meta::is_template(m);
    static_assert(t || !t);
  }
}

int main() { walk(); }
