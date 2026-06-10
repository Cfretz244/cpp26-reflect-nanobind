// TC-0003 — member-kind metafunctions ICE on entity proxies (using-shadow declarations).
// Standalone repro (no nanobind). See corpus/findings/TC-0003-entity-proxy-metafunction-ice.md.
//
// With -fentity-proxy-reflection, members_of enumerates using-shadow declarations as
// ReflectionKind::EntityProxy. Six metafunctions left that kind as
// llvm_unreachable("proxies should already have been unwrapped"), so enumerating a
// class's members and asking an ordinary kind question ICEs the compiler:
//
//   is_constructor (default below), is_destructor, is_special_member_function,
//   is_static_member, is_enumerable_type, has_complete_definition
//
// Pick which one to demonstrate with -DPROBE_<NAME>; each independently crashes an
// unfixed compiler. The other unreachable arms are NOT reachable from user code
// (identifier_of/has_identifier/source_location_of pre-unwrap; substitute's dispatcher
// unwraps template arguments; reflect_invoke through a proxy fails gracefully).
//
// Expected: compiles (the queries answer false for a proxy — a shadow declaration is
// never itself a constructor/destructor/etc.; ask underlying_entity_of(m) for the
// target's properties).
//
// Actual at bloomberg/clang-p2996 @ 837da39eb88c:
//   "proxies should already have been unwrapped"
//   UNREACHABLE executed at clang/lib/AST/ExprConstantMeta.cpp:<line of the queried fn>!
//
// Build (from the umbrella repo root; -fsyntax-only suffices):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -fentity-proxy-reflection \
//     -stdlib=libc++ -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fsyntax-only repro.cpp
#include <experimental/meta>

namespace meta = std::meta;

struct B {
  int f() const { return 1; }
  static int g() { return 2; }
  static int s;
  int field;
  using T = int;
  struct N {};
};

struct D : private B {
  using B::f;
  using B::g;
  using B::s;
  using B::field;
  using B::T;
  using B::N;
};

consteval bool query_proxies() {
  for (auto m : meta::members_of(^^D, meta::access_context::unchecked())) {
    if (!meta::is_entity_proxy(m))
      continue;
#if defined(PROBE_IS_DESTRUCTOR)
    if (meta::is_destructor(m)) return false;
#elif defined(PROBE_IS_SPECIAL_MEMBER)
    if (meta::is_special_member_function(m)) return false;
#elif defined(PROBE_IS_STATIC_MEMBER)
    if (meta::is_static_member(m)) return false;
#elif defined(PROBE_IS_ENUMERABLE_TYPE)
    if (meta::is_enumerable_type(m)) return false;
#elif defined(PROBE_HAS_COMPLETE_DEFINITION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (meta::has_complete_definition(m)) return false;
#pragma clang diagnostic pop
#else
    if (meta::is_constructor(m)) return false;  // ICEs on the first proxy
#endif
  }
  return true;
}
static_assert(query_proxies());

int main() { return 0; }
