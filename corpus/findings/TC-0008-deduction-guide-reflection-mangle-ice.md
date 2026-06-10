# TC-0008 — mangling a deduction-guide reflection ICEs: "Can't mangle a deduction guide name!"

- **Status:** FIXED (toolchain + binder belt-and-suspenders) — fixed in the
  pinned llvm-project: `mangleReflection`'s Template case now encodes a guide
  as `"dg"` + the deduced template's name + the TC-0004 `'$'`-bracketed ODR
  hash, with `isImplicit()` + the deduction-candidate kind folded in (Sema's
  implicit per-constructor guide is structurally identical to a
  same-signature explicit guide, and implicit guides enumerate alongside
  explicit ones once CTAD is used). Regression test
  `deduction-guide-reflection-mangling.pass.cpp`. The binder ALSO strips
  guides from its namespace walks before the `define_static_array` lift
  (`namespace_members_for_binding` in nb_reflect.h — a guide is never
  bindable), so it works on unpatched toolchains too. Upstream:
  `repros/TC-0008/UPSTREAM.md`.
- **Track:** toolchain (clang-p2996, pinned by this repo), Itanium mangler —
  same component family as TC-0004 (`mangleReflection`), different
  declaration-name kind
- **Found via:** `corpus/runs/expected` (TartanLlama/expected v1.3.1). This is
  the run's RECORDED Gate-4 failure: compiling the binding TU to an object
  aborts (exit 134) with
  `Can't mangle a deduction guide name! UNREACHABLE executed at
  clang/lib/AST/ItaniumMangle.cpp:1774`.
- **Repro:** `corpus/findings/repros/TC-0008/repro.cpp` (standalone; needs `-c`,
  `-fsyntax-only` is clean — it is the mangler).

## Symptom

`members_of` over a namespace enumerates deduction guides like any other
member. Lifting that member list into static storage —

```cpp
template for (constexpr auto m :
    std::define_static_array(std::meta::members_of(^^demo,
        std::meta::access_context::unchecked()))) { ... }
```

— gives the backing `std::meta::__define_static::FixedArray` specialization a
linkage name in which each element reflection is mangled as a template
argument (`CXXNameMangler::mangleReflection`). A reflection of a *template*
mangles the template's name (`mangleTemplateName` →
`mangleUnqualifiedName`); a deduction guide's `DeclarationName` is
`CXXDeductionGuideNameKind`, which `mangleUnqualifiedName` does not handle:

```
Can't mangle a deduction guide name!
UNREACHABLE executed at clang/lib/AST/ItaniumMangle.cpp:1774
  #8 CXXNameMangler::mangleUnqualifiedName
  #9 CXXNameMangler::mangleTemplatePrefix
 #10 CXXNameMangler::mangleTemplateName
 #11 CXXNameMangler::mangleReflection
```

Controls: removing the deduction guide compiles clean; `-fsyntax-only` with the
guide is clean (front-end handles the reflection fine — only mangling dies).

## Field shape

tl/expected.hpp declares at namespace scope (always on at C++26):

```cpp
template <class E> unexpected(E) -> unexpected<E>;   // #ifdef __cpp_deduction_guides
```

The binder's `bind_free_operators` pass walks `parent_of(^^T)` with exactly the
`define_static_array(members_of(...))` shape while binding ANY class in `tl`,
so every `tl::expected`/`tl::unexpected` binding ICEs at codegen. There is no
consumer-side dodge (the guide is unconditionally in the namespace).

## Layering in corpus/runs/expected

This ICE preempts (at `-c`) the BINDER-0012 hard error (deleted default ctor of
`tl::unexpected<E>` bound), which is what `-fsyntax-only` shows; behind both,
`expected<void, std::string>` was already dropped for TC-0006/TC-0007. Fix
order for the run to go green: TC-0008 (or a binder-side deduction-guide
filter in the namespace walks — also reasonable on its own: a guide is never
bindable), then BINDER-0012.
