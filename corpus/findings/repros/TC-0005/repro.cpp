// TC-0005 — function-type metafunctions are blind to AttributedType sugar:
// [[clang::lifetimebound]] methods misreport all qualifiers; their TYPE is
// rejected by return_type_of/parameters_of.
// Standalone repro (no nanobind). See
// corpus/findings/TC-0005-attributed-function-type-predicate-misreport.md.
//
// An attribute on a function declarator (e.g. [[clang::lifetimebound]] on the
// implicit object parameter -- Abseil puts it on every accessor via
// ABSL_ATTRIBUTE_LIFETIME_BOUND) wraps the declaration's FunctionProtoType in
// AttributedType sugar. Metafunctions reaching the FunctionProtoType with a
// sugar-blind dyn_cast misbehave:
//   - is_const / is_volatile / is_lvalue_reference_qualified /
//     is_rvalue_reference_qualified silently answer FALSE (wrong answers, no
//     diagnostic) -- on the declaration, on its type, and through an entity
//     proxy's underlying entity;
//   - return_type_of / parameters_of / has_ellipsis_parameter on the
//     function's TYPE reject it as not-a-function-type (fails to be a constant
//     expression).
// is_noexcept always worked: isFunctionOrMethodNoexcept desugars via getAs.
//
// Expected: compiles (all static_asserts hold).
// Actual at bloomberg/clang-p2996 @ 837da39eb88c: the qualifier static_asserts
// FAIL (predicates answer false), and the -DTYPE_QUERIES block fails with
// "call to consteval function ... is not a constant expression".
//
// Field shape: every qualifier predicate reports false for all four of
// absl::StatusOr<int>::value()'s overloads (const& / & / const&& / &&), seen
// through their using-redeclaration proxies -- originally mis-filed as an
// entity-proxy bug (TC-0003 addendum); minimization showed proxies are
// incidental and ABSL_ATTRIBUTE_LIFETIME_BOUND is the trigger.
//
// Build (from the umbrella repo root; -fsyntax-only suffices):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fsyntax-only repro.cpp [-DTYPE_QUERIES]
#include <experimental/meta>
#include <string_view>

namespace meta = std::meta;

consteval meta::info member_named(meta::info cls, std::string_view name) {
  for (auto m : meta::members_of(cls, meta::access_context::unchecked()))
    if (meta::has_identifier(m) && meta::identifier_of(m) == name)
      return m;
  return {};
}

struct W {
  const int& cl() const & [[clang::lifetimebound]] { return v; }
  int& l() & [[clang::lifetimebound]] { return v; }
  const int&& cr() const && [[clang::lifetimebound]] {
    return static_cast<const int&&>(v);
  }
  int&& r() && [[clang::lifetimebound]] { return static_cast<int&&>(v); }
  int v;
};

// Control: without the attribute, everything answers correctly (drop
// [[clang::lifetimebound]] above and these continue to hold).
static_assert(meta::is_const(member_named(^^W, "cl")));                       // FAILS at base
static_assert(meta::is_lvalue_reference_qualified(member_named(^^W, "cl"))); // FAILS at base
static_assert(meta::is_lvalue_reference_qualified(member_named(^^W, "l")));  // FAILS at base
static_assert(meta::is_const(member_named(^^W, "cr")));                      // FAILS at base
static_assert(meta::is_rvalue_reference_qualified(member_named(^^W, "cr"))); // FAILS at base
static_assert(meta::is_rvalue_reference_qualified(member_named(^^W, "r")));  // FAILS at base

#if defined(TYPE_QUERIES)
// The function's TYPE carries the sugar too: these are rejected outright
// ("not a constant expression") at base.
static_assert(
    meta::return_type_of(meta::type_of(member_named(^^W, "l"))) == ^^int&);
static_assert(meta::parameters_of(meta::type_of(member_named(^^W, "l"))).size() == 0);
static_assert(!meta::has_ellipsis_parameter(meta::type_of(member_named(^^W, "l"))));
#endif

int main() { return 0; }
