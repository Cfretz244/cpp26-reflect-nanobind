## Summary

`members_of` over a namespace enumerates deduction guides like any other member. Lifting that member list into static storage with `define_static_array` gives the backing array specialization a linkage name in which each element reflection is mangled as a template argument (`CXXNameMangler::mangleReflection`). A reflection of a *template* is encoded by the template's name (`mangleTemplateName` → `mangleTemplatePrefix` → `mangleUnqualifiedName`) — but a deduction guide's `DeclarationName` is `CXXDeductionGuideName`, which has no `<unqualified-name>` encoding:

```
Can't mangle a deduction guide name!
UNREACHABLE executed at clang/lib/AST/ItaniumMangle.cpp:1774
  #8 CXXNameMangler::mangleUnqualifiedName
  #9 CXXNameMangler::mangleTemplatePrefix
 #10 CXXNameMangler::mangleTemplateName
 #11 CXXNameMangler::mangleReflection
```

That unreachable is a perfectly sound invariant for normal symbol mangling (guides are never odr-used) — but reflection makes guide *reflections* reachable in mangled-name position from ordinary user code.

A wrinkle that shapes the fix: once CTAD has been used in the TU, Sema's **implicit** guides (per-constructor + the copy guide) are enumerated alongside explicit ones, every guide for one template shares the single `CXXDeductionGuideName`, and an implicit per-constructor guide can be *structurally identical* to a same-signature explicit guide — so the encoding must discriminate all of them, not just avoid the crash.

## Field evidence

TartanLlama/expected v1.3.1 declares at namespace scope (always on at C++26):

```cpp
template <class E> unexpected(E) -> unexpected<E>;   // behind #ifdef __cpp_deduction_guides
```

A reflection-driven binding generator walks `members_of(parent_of(^^T))` through `define_static_array` while binding ANY class in `tl`, so every `tl::expected`/`tl::unexpected` binding ICEs at codegen (exit 134). There is no consumer-side dodge: the guide is unconditionally in the namespace.

## Reproducer

Self-contained, `-std=c++26 -freflection-latest`:

```cpp
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
```

Build matrix:

- `clang++ -DGUIDE -c repro.cpp` — **ICE** (the bug; needs codegen)
- `clang++ -c repro.cpp` — clean (control: no guide)
- `clang++ -DGUIDE -fsyntax-only` — clean (the front-end is fine; it is the mangler)

## Expected

Compiles and runs; each guide reflection gets a deterministic, distinct mangling.

## Actual (at `837da39eb88c`)

The unreachable above, exit 134.

## Suggested fix (PR follows shortly)

Local to `mangleReflection`'s `ReflectionKind::Template` case (the `mangleUnqualifiedName` unreachable stays): encode `"dg"` + the *deduced* template's name + the same `'$'`-bracketed ODR-hash discriminator used for overloaded function templates (#286), with `isImplicit()` and the deduction-candidate kind folded into the hash so explicit guides, same-signature implicit per-constructor guides, and copy guides all stay pairwise distinct. Deterministic and cross-TU-stable, preserving legitimate linkonce_odr merging; distinct from a reflection of the deduced class template itself. The regression test pins the lift shape plus four guides (2 explicit + 2 implicit) as `&probe<m>` NTTPs and asserts pairwise distinctness at runtime.

Note: the PR stacks on #287 (it extends the same `mangleReflection` hash block).
