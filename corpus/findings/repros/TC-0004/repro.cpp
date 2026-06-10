// TC-0004 — substitute() results misreport predicates under nested-dependent instantiation.
// Standalone repro (no nanobind). See corpus/findings/TC-0004-nested-dependent-substitute-misreport.md.
//
// Shape: a struct with TWO member operator[] templates — one instantiable with all-defaulted
// parameters, one a SFINAE-false pack sibling (mirrors absl raw_hash_map's lifetimebound
// operator[] pair). A function template (level 1) walks members_of(^^T) with `template for`
// and dispatches each function template to another function template (level 2), which calls
// std::meta::substitute(tmpl, {}) and queries predicates on the result.
//
// Expected: at every level, the instantiable template substitutes and reports
//   is_operator_function=1 is_volatile=0 is_rvalue_reference_qualified=0
// and the pack sibling reports can_substitute=0.
//
// Actual at the BROKEN pin (llvm-project @ 651ff7a): correct at levels 0/1, but at level 2
// BOTH calls printed "member#1 can_sub=0" -- the predicates never misreported on a real
// spec; the two l2_inner<B_l2, fn> instantiations were FOLDED into one at codegen (their
// info NTTPs, naming two same-named operator[] templates, mangled identically:
// __Z8l2_innerI4B_l2MtNS0_ixIEEEEvi for both; the AST-level specializations were correct
// and distinct) and the SFINAE-false sibling's body served both call sites. That is the
// silent-no-diagnostic operator[] drop the binder observed. Fixed in the llvm-project
// submodule (ItaniumMangle.cpp, ReflectionKind::Template: append an ODR hash of the
// template head + pattern for FunctionTemplateDecls); on a fixed toolchain all four
// sections print identical, correct results and `nm` shows two distinct l2_inner symbols.
//
// Four identical structs (macro-stamped) keep the variants from sharing specialization
// state: B_l0 (non-dependent baseline), B_l1 (one dependent level), B_l2 (the failing
// two-level shape), B_nttp (two-level + level-1 spec passed down as NTTP = the binder's
// workaround, including an identity probe spec==spec_l1).
//
// NOTE: the level-2 dispatch target deliberately has ONLY <T, tmpl> as template parameters
// (an extra discriminating NTTP, e.g. an index, would mask the collision). The two
// instantiations differ only in a std::meta::info NTTP naming two same-named operator[]
// templates — under the broken mangling they collapse (check: both calls print the same
// member# / can_sub, and `nm` shows a single l2_inner<B_l2, ...> symbol).
//
// Build & run (from the umbrella repo root):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
//     -L $TC/lib -Wl,-rpath,$TC/lib \
//     corpus/findings/repros/TC-0004/repro.cpp -o corpus/findings/repros/TC-0004/repro
//   DYLD_LIBRARY_PATH=$TC/lib ./corpus/findings/repros/TC-0004/repro

#include <meta>

#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template <bool C> using EnableIf = std::enable_if_t<C, int>;
template <class K, bool V> inline constexpr bool LTB = !V;

// The TC-0004 shape: instantiable operator[] template + SFINAE-false pack sibling.
#define DEFINE_B(NAME)                                                                  \
  struct NAME {                                                                         \
    template <class K = int, class P = double, int = EnableIf<LTB<K, false>>()>         \
    std::string& operator[](K&& k);                                                     \
    template <class K = int, class P = double, int&..., EnableIf<LTB<K, true>> = 0>     \
    std::string& operator[](K&& k);                                                     \
  }

DEFINE_B(B_l0);
DEFINE_B(B_l1);
DEFINE_B(B_l2);
DEFINE_B(B_nttp);

// consteval wrappers around can_substitute/substitute with zero arguments, exactly as
// the binder's fn_template_default_instantiable does (calling the metafunctions directly
// in an if-constexpr condition is rejected; they must run inside an immediate function).
consteval bool can_sub0(std::meta::info tmpl) {
  return std::meta::can_substitute(tmpl, std::vector<std::meta::info>{});
}
consteval std::meta::info sub0(std::meta::info tmpl) {
  return std::meta::substitute(tmpl, std::vector<std::meta::info>{});
}

// Position of `fn` within members_of(cls) — identity probe that survives identical
// display strings (both templates display as "operator[]").
consteval int member_index(std::meta::info cls, std::meta::info fn) {
  int i = 0;
  for (auto m : std::meta::members_of(cls, std::meta::access_context::unchecked())) {
    if (m == fn) return i;
    ++i;
  }
  return -1;
}

static void print_preds(int call, int mindex, bool op, bool vol, bool rref,
                        std::string_view fnty) {
  std::printf("  [call %d] member#%d can_sub=1 is_op=%d is_volatile=%d is_rref=%d type=%.*s\n",
              call, mindex, op, vol, rref, (int)fnty.size(), fnty.data());
}

static void print_nosub(int call, int mindex) {
  std::printf("  [call %d] member#%d can_sub=0\n", call, mindex);
}

// ---------------------------------------------------------------- level 0 (non-dependent)
void level0() {
  int call = 0;
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^B_l0, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(fn)) {
      constexpr int mi = member_index(^^B_l0, fn);
      if constexpr (can_sub0(fn)) {
        constexpr auto spec = sub0(fn);
        constexpr bool op   = std::meta::is_operator_function(spec);
        constexpr bool vol  = std::meta::is_volatile(spec);
        constexpr bool rref = std::meta::is_rvalue_reference_qualified(spec);
        constexpr auto ty   = std::meta::display_string_of(std::meta::type_of(spec));
        print_preds(call++, mi, op, vol, rref, ty);
      } else {
        print_nosub(call++, mi);
      }
    }
  }
}

// ----------------------------------------------------- level 1 (one dependent level only)
template <typename T>
void level1_direct() {
  int call = 0;
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(fn)) {
      constexpr int mi = member_index(^^T, fn);
      if constexpr (can_sub0(fn)) {
        constexpr auto spec = sub0(fn);
        constexpr bool op   = std::meta::is_operator_function(spec);
        constexpr bool vol  = std::meta::is_volatile(spec);
        constexpr bool rref = std::meta::is_rvalue_reference_qualified(spec);
        constexpr auto ty   = std::meta::display_string_of(std::meta::type_of(spec));
        print_preds(call++, mi, op, vol, rref, ty);
      } else {
        print_nosub(call++, mi);
      }
    }
  }
}

// ------------------------------------- level 2 (the failing shape: substitute two levels deep)
template <typename T, std::meta::info tmpl>
void l2_inner(int call) {
  constexpr int mi = member_index(^^T, tmpl);
  if constexpr (can_sub0(tmpl)) {
    constexpr auto spec = sub0(tmpl);
    constexpr bool op   = std::meta::is_operator_function(spec);
    constexpr bool vol  = std::meta::is_volatile(spec);
    constexpr bool rref = std::meta::is_rvalue_reference_qualified(spec);
    constexpr auto ty   = std::meta::display_string_of(std::meta::type_of(spec));
    print_preds(call, mi, op, vol, rref, ty);
  } else {
    print_nosub(call, mi);
  }
}

template <typename T>
void l2_outer() {
  int call = 0;
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(fn))
      l2_inner<T, fn>(call++);
  }
}

// ------------------- level 2 with the level-1 spec passed down as NTTP (workaround shape)
template <typename T, std::meta::info tmpl, std::meta::info spec_l1>
void nt_inner(int call) {
  constexpr int mi = member_index(^^T, tmpl);
  if constexpr (can_sub0(tmpl)) {
    constexpr auto spec = sub0(tmpl);
    constexpr bool same = (spec == spec_l1);
    constexpr bool op   = std::meta::is_operator_function(spec);
    constexpr bool vol  = std::meta::is_volatile(spec);
    constexpr bool rref = std::meta::is_rvalue_reference_qualified(spec);
    constexpr bool op1  = std::meta::is_operator_function(spec_l1);
    constexpr auto ty   = std::meta::display_string_of(std::meta::type_of(spec));
    print_preds(call, mi, op, vol, rref, ty);
    std::printf("           spec==spec_l1: %d   is_op(spec_l1): %d\n", same, op1);
  } else {
    print_nosub(call, mi);
  }
}

template <typename T>
void nt_outer() {
  int call = 0;
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (std::meta::is_function_template(fn)) {
      constexpr int mi = member_index(^^T, fn);
      if constexpr (can_sub0(fn)) {
        constexpr auto spec1 = sub0(fn);
        constexpr bool op1   = std::meta::is_operator_function(spec1);
        std::printf("  level1: member#%d substituted, is_op=%d -> dispatching\n", mi, op1);
        nt_inner<T, fn, spec1>(call++);
      } else {
        print_nosub(call++, mi);
      }
    }
  }
}

int main() {
  std::puts("== level 0 (non-dependent) ==");
  level0();
  std::puts("== level 1 (template for directly in a function template) ==");
  level1_direct<B_l1>();
  std::puts("== level 2 (substitute inside nested dependent instantiation -- failing shape) ==");
  l2_outer<B_l2>();
  std::puts("== level 2 + level-1 spec as NTTP (binder workaround shape) ==");
  nt_outer<B_nttp>();
}
