# DRAFT — not yet filed (bloomberg/clang-p2996)

Candidate issue title:
**`can_substitute`/`substitute` crash (SIGSEGV) instead of reporting substitution
failure when substitution forms an invalid type inside a template-id (e.g.
reference to void)**

## Summary

`std::meta::can_substitute(tmpl, args)` is the SFINAE probe of P2996: a
substitution that fails in the immediate context should make it return `false`.
When the failing substitution forms an invalid *type* inside a template-id —
`trait<OT&>` with `OT=void`, the classic reference-to-void deduction failure —
the frontend instead SIGSEGVs inside
`(anonymous namespace)::MetaActionsImpl::Substitute(clang::FunctionTemplateDecl*, ...)`
(reached from `clang::Metafunction::evaluate` / `ReflectionEvaluator`).

## Repro

`repro.cpp` in this directory; `-fsyntax-only` suffices:

```
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -fsyntax-only repro.cpp
```

Expected: compiles (the `^^void` probe asserts `false`, the `^^int` control
asserts `true`). Actual: exit 139, stack dump ending in
`MetaActionsImpl::Substitute`.

Both forms crash:
- free function template + explicit `{^^void}` argument;
- member function template with a defaulted parameter picking up the enclosing
  specialization's `void` argument, probed with `can_substitute(member, {})`.

Negative observations (from minimization):
- `decltype(std::declval<OT&>())` as a template-PARAMETER default argument does
  not crash (failure is reported correctly); the invalid type must be formed in
  the declaration proper (return type / nested-name `enable_if` position).
- Not order-dependent; pre-instantiating the enclosing class does not help
  (unlike the companion finding TC-0007).

## Field shape

`tl::expected<void, E>` (TartanLlama/expected v1.3.1):

```cpp
template <class OT = T, class OE = E>
detail::enable_if_t<detail::is_swappable<OT>::value && ...> swap(expected&) noexcept(...);
```

`detail::is_swappable<void>` (tl's C++11 `swap_adl_tests`,
`decltype(swap(std::declval<T&>(), std::declval<U&>()))`) is plain `false` in
ordinary C++; probing the member with `can_substitute({})` — what a
reflection-driven binding generator does to every member template — crashes the
compiler, so `expected<void, E>` cannot be reflected over at all.
