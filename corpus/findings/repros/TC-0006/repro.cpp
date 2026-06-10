// TC-0006 — can_substitute / substitute SEGV when substitution forms an
// invalid type inside a template-id (reference to void) instead of reporting
// substitution failure.
// Standalone repro (no nanobind, no library). See
// corpus/findings/TC-0006-can-substitute-invalid-type-formation-segv.md.
//
// can_substitute is the SFINAE probe of P2996: a substitution that fails in
// the immediate context is supposed to make it return false (and make
// substitute() a non-constant evaluation), not crash. Substituting OT=void
// here must form `trait<OT&>` -> `void&`, exactly the classic
// reference-to-void deduction failure every enable_if-era SFINAE library
// leans on. clang-p2996 instead crashes with SIGSEGV inside
// (anonymous namespace)::MetaActionsImpl::Substitute(FunctionTemplateDecl*, ...)
// reached from clang::Metafunction::evaluate / ReflectionEvaluator.
//
// Expected: compiles (both static_asserts hold: the void substitution reports
// failure, the int control succeeds).
// Actual at clang-p2996 (toolchain pinned by this repo, base 837da39eb88c
// family): clang frontend SIGSEGV (exit 139), stack dump ends in
// MetaActionsImpl::Substitute.
//
// Control: the same trait machinery WITHOUT reflection is ordinary SFINAE --
// e.g. overload resolution simply discards such a candidate (see the field
// shape below); and can_substitute over the same templates with non-void
// arguments is fine.
//
// Field shape: tl::expected<void, E> (TartanLlama/expected v1.3.1). Its
//   template <class OT = T, class OE = E>
//   detail::enable_if_t<detail::is_swappable<OT>::value && ...> swap(expected&)
// member template hides reference-to-void formation (declval<OT&>) behind
// detail::is_swappable<OT=void>. The reflection-driven nanobind binder probes
// every member template with can_substitute({}) to decide default-
// instantiability, so MERELY REFLECTING over expected<void, E>'s members
// crashes the compiler. Plain C++ use of expected<void, E> (including
// is_swappable<void>::value == false) compiles fine.
//
// Build (from the umbrella repo root; -fsyntax-only suffices):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fsyntax-only repro.cpp
#include <experimental/meta>
#include <vector>

namespace meta = std::meta;

template <class T> struct trait { using type = void; };

// Substituting OT=void forms trait<void&>: invalid type in the immediate
// context => substitution failure, NOT a crash.
template <class OT> typename trait<OT&>::type f() {}

// The field shape: a member template whose DEFAULTED parameter picks up the
// enclosing specialization's void argument (tl::expected<void,E>::swap).
template <class T> struct Exp {
  template <class OT = T>
  typename trait<OT&>::type swap(Exp&) {}
};

consteval bool probe_member_templates(meta::info cls) {
  for (auto mem : meta::members_of(cls, meta::access_context::unchecked()))
    if (meta::is_function_template(mem))
      (void)meta::can_substitute(mem, std::vector<meta::info>{});
  return true;
}

// Control: non-void substitution works.
static_assert(meta::can_substitute(^^f, std::vector<meta::info>{^^int}));

// Crash #1: explicit void argument on a free function template.
static_assert(!meta::can_substitute(^^f, std::vector<meta::info>{^^void}));

// Crash #2: the defaulted-parameter member-template form (the binder's walk).
static_assert(probe_member_templates(^^Exp<void>));

int main() {}
