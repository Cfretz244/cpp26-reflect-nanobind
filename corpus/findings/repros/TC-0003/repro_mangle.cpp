// TC-0003 (part 2) — Itanium mangler defects on entity-proxy reflections as template
// arguments. Standalone repro (no nanobind).
// See corpus/findings/TC-0003-entity-proxy-metafunction-ice.md.
//
// With -fentity-proxy-reflection, an EntityProxy reflection used as a std::meta::info
// NTTP must be mangled. mangleReflection's EntityProxy case mangled the shadow
// declaration's NAME via mangleNameWithAbiTags. Two defects follow:
//
//   1. ICE (default below): an operator-named shadow (`using OB::operator*;`) crashes —
//      mangleUnqualifiedName casts the decl to FunctionDecl to disambiguate the operator
//      name, and a UsingShadowDecl is not one:
//        Assertion failed: (isa<To>(Val) && "cast<Ty>() argument of incompatible
//        type!"), ... CXXNameMangler::mangleUnqualifiedName
//      No class template required. This is the shape absl::StatusOr<T> hits via
//      `using StatusOr::OperatorBase::operator*;` (originally mis-attributed to the
//      shadow living in a class template specialization).
//
//   2. Collision (-DCOLLIDE): one using-declarator over an overload set introduces
//      several same-named shadows; their proxies all mangle identically:
//        error: definition with same mangled name '_Z5probeIMaN1SIiE5valueE$EEiv' as
//        another definition
//      (a TC-0004-style fold risk surfacing as a hard error here).
//
// Expected: compiles and runs, exit 0 — proxy reflections mangle via their TARGET
// declaration (each shadow's own overload / operator), keeping the proxy tag so a
// proxy-of-X reflection stays a distinct template argument from declaration-of-X.
//
// Actual at bloomberg/clang-p2996 @ 837da39eb88c: the cast<> assertion (default) or the
// mangled-name collision error (-DCOLLIDE). Must reach CodeGen: -fsyntax-only does NOT
// reproduce.
//
// Build & run (from the umbrella repo root):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -fentity-proxy-reflection \
//     -stdlib=libc++ -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -L $TC/lib -Wl,-rpath,$TC/lib \
//     repro_mangle.cpp -o repro_mangle && ./repro_mangle
#include <experimental/meta>

namespace meta = std::meta;

#if defined(COLLIDE)
template <class T> struct OB {
  int value() & { return 1; }
  int value() const & { return 2; }
  int value() && { return 3; }
  int value() const && { return 4; }
};
template <class T> struct S : private OB<T> { using OB<T>::value; };
constexpr int expected = 4;
#define TARGET S<int>
#else
struct OB {
  int operator*() const { return 1; }
};
struct S : private OB { using OB::operator*; };
constexpr int expected = 1;
#define TARGET S
#endif

template <std::meta::info M> int probe() { return 7; }

int main() {
  int found = 0;
  template for (constexpr auto m : std::define_static_array(
          meta::members_of(^^TARGET, meta::access_context::unchecked()))) {
    if constexpr (meta::is_entity_proxy(m)) {
      probe<m>();  // mangling probe<M> must encode the proxy reflection
      ++found;
    }
  }
  return found == expected ? 0 : 1;
}
