// TC-0009 — reflections of same-HEADED member function templates of a class
// template SPECIALIZATION mangle identically as NTTPs: the TC-0004
// discriminator hashes the declaration via ODRHash::AddFunctionDecl, which
// silently NO-OPS for any declaration in "specialization context" (member of
// a ClassTemplateSpecializationDecl -- the common members_of shape), leaving
// only the (identical) template heads in the hash.
// Standalone repro (no nanobind, no library). See
// corpus/findings/TC-0009-same-headed-member-template-nttp-mangling.md.
//
// Symptom: "error: definition with same mangled name ... as another
// definition" when each sibling's reflection instantiates a dispatcher
// (probe<m>) -- or, when the TU gets away with it, a silent linkonce_odr fold
// where ONE body serves all call sites (the TC-0004 failure mode, new
// trigger). The AST-level specializations are correct and distinct.
//
// Field shape: tl::expected<int, std::string>'s four value() member templates
// (const& / & / const&& / &&) share one template head
//   template <class U = T, detail::enable_if_t<!is_void<U>::value>* = nullptr>
// and differ only in cv/ref qualifiers + return type. The reflection-driven
// nanobind binder dispatches each through reflect_bind_member_template<T,
// tmpl>; all four reflections mangled identically, codegen folded the four
// dispatch bodies into one, and value() silently never bound -- found by the
// corpus/runs/expected differential suite on its FIRST post-TC-0008 execution.
//
// Build matrix (from the umbrella repo root):
//   TC=$PWD/toolchain
//   CXX="$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot $(xcrun --show-sdk-path) -nostdinc++ -isystem $TC/include/c++/v1"
//   $CXX -c repro.cpp -o /tmp/r.o          # error: definition with same mangled name (the bug)
//   $CXX -fsyntax-only repro.cpp           # clean (front-end is fine; it is the mangler)
//   $CXX -DFREE_CONTROL -c repro.cpp       # clean (control: same shape at namespace
//                                          #        scope is NOT in specialization context)
#include <experimental/meta>

namespace meta = std::meta;

template <class T> struct trait { static constexpr bool value = true; };

#ifndef FREE_CONTROL
// Same-headed qualifier siblings as members of a class template -- their
// reflections come from members_of over the SPECIALIZATION Exp<int>.
template <class T> struct Exp {
  template <class U = T, bool = trait<U>::value>
  const U &value() const & { return v; }
  template <class U = T, bool = trait<U>::value>
  U &value() & { return v; }
  T v;
};
#define SCOPE ^^Exp<int>
#else
// Control: the same two signatures at namespace scope (no specialization
// context) -- AddFunctionDecl hashes them, manglings differ, compiles clean.
namespace ctl {
template <class U = int, bool = trait<U>::value> const U &value();
template <class U = int, bool = trait<U>::value> U *value(int);
}
#define SCOPE ^^ctl
#endif

template <meta::info R> int probe() { return 1; }

int main() {
  int (*addrs[8])() = {};
  int n = 0;
  template for (constexpr auto m : std::define_static_array(
      meta::members_of(SCOPE, meta::access_context::unchecked()))) {
    if constexpr (meta::is_function_template(m)) {
      addrs[n++] = &probe<m>;
    }
  }
  return n == 2 && addrs[0] != addrs[1] ? 0 : 1;
}
