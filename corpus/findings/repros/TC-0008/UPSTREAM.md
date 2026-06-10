# DRAFT — not yet filed (bloomberg/clang-p2996)

Candidate issue title:
**Itanium mangler ICE ("Can't mangle a deduction guide name!") mangling a
reflection of a deduction guide as a template argument /
`define_static_array` element**

## Summary

A deduction guide is an enumerable namespace member under `members_of`. When a
reflection of one ends up in mangled-name position — e.g. as an element of a
`std::define_static_array(members_of(^^ns, ...))` backing array, whose
`FixedArray` specialization mangles each element reflection via
`CXXNameMangler::mangleReflection` — the mangler encodes the reflected
template by NAME (`mangleTemplateName` → `mangleUnqualifiedName`), and
`CXXDeductionGuideNameKind` hits the `llvm_unreachable` at
`clang/lib/AST/ItaniumMangle.cpp:1774`.

Front-end handling is fine (`-fsyntax-only` clean); only codegen/mangling
crashes. Same component as the (fixed) TC-0004 issue — `mangleReflection` on
`ReflectionKind::Template` — but a different declaration-name kind: TC-0004
covered same-named function templates folding, this is a name kind
`mangleUnqualifiedName` cannot encode at all.

## Repro

`repro.cpp` in this directory:

```
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -DGUIDE -c repro.cpp        # ICE
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -c repro.cpp                # clean (no guide)
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -DGUIDE -fsyntax-only ...   # clean (mangler-only)
```

## Field shape

TartanLlama/expected declares `template <class E> unexpected(E) ->
unexpected<E>;` at namespace scope. A reflection-driven binding generator that
walks `members_of(^^tl, ...)` through `define_static_array` (nanobind's
reflection binder does, for free-operator discovery) ICEs while binding ANY
class in that namespace. Possible fixes: mangle the guide via its deduced
template (`a`-tag + the template it guides, kind-aware like the TC-0004 fix),
or at minimum a proper diagnostic instead of `llvm_unreachable`.
