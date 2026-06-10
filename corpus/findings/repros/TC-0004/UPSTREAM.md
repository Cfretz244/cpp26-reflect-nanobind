# FILED — bloomberg/clang-p2996 issue + PR (2026-06-09)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/286
- **PR:** https://github.com/bloomberg/clang-p2996/pull/287 — branch
  `reflect-fn-template-nttp-mangling` on `Cfretz244/llvm-project` (commit `caac148`:
  `b329d544cb20` cherry-picked onto their `p2996` tip `837da39` — which equaled our local
  base, so the PR code is byte-for-byte what the corpus validated — with the internal
  TC-0004 reference dropped from the code comment).

The draft below is what was filed (lightly adapted in the issue itself).

---

---

## Title

Reflections of same-named (overloaded) function templates mangle identically as template
arguments; CodeGen silently folds the resulting specializations

## Summary

`mangleReflection`'s `ReflectionKind::Template` case encodes only the reflected
template's *name*. Function templates can be overloaded, so reflections of two same-named
sibling templates are mangled byte-identically. A function template taking such a
reflection as an NTTP then gets one mangled name for two distinct specializations; the
AST is correct, but CodeGen folds the linkonce_odr definitions by mangled name and a
single body silently serves both call sites. No diagnostic is emitted at any point.

This bites any reflection-driven dispatcher that walks `members_of(^^T)` with
`template for` and forwards each member to a helper as an NTTP — the natural shape for
binding generators. In the wild: `absl::flat_hash_map<K,V>` exposes `operator[]` as a
member-template pair (an instantiable overload plus a SFINAE-false lifetimebound
sibling); a nanobind-style binder bound the map's entire heterogeneous query surface
except `operator[]`, which vanished without any diagnostic because the sibling's
(no-op) dispatch body was the one that survived the fold.

## Reproducer

`repro.cpp` (attached; self-contained, `-std=c++26 -freflection-latest`, no extensions
beyond that umbrella flag). Core shape:

```cpp
template <bool C> using EnableIf = std::enable_if_t<C, int>;
template <class K, bool V> inline constexpr bool LTB = !V;

struct B {
  template <class K = int, class P = double, int = EnableIf<LTB<K, false>>()>
  std::string& operator[](K&& k);                       // default-instantiable
  template <class K = int, class P = double, int&..., EnableIf<LTB<K, true>> = 0>
  std::string& operator[](K&& k);                       // SFINAE-false pack sibling
};

template <typename T, std::meta::info tmpl>
void level2(int call) {                                  // <-- NTTP dispatcher
  /* can_substitute / substitute(tmpl, {}) + print which member this body is for */
}

template <typename T>
void level1() {
  template for (constexpr auto fn : std::define_static_array(
          std::meta::members_of(^^T, std::meta::access_context::unchecked())))
    if constexpr (std::meta::is_function_template(fn))
      level2<T, fn>(call++);
}
```

## Expected

Two distinct `level2<B, ...>` specializations; call 0 reports the first `operator[]`
(substitutable, `is_operator_function == true`), call 1 reports the pack sibling
(`can_substitute == false`).

## Actual

Both calls execute the SAME body (whichever the fold kept — observed: the sibling's), so
e.g. both report `member#1 can_sub=0`. `nm` on the object shows a single
`__Z8l2_innerI4B_l2MtNS0_ixIEEEEvi` — the `Mt...` reflection mangling carries no
information distinguishing the overloads. `-ast-dump` confirms two correct, distinct
`FunctionDecl` specializations with the right per-body constants; the loss is at
mangling/CodeGen.

Notes that helped us misdiagnose it initially (may save a triager time):

- It masquerades as "`substitute()`/predicate misreports two dependent levels deep": the
  observable is a consteval predicate apparently answering wrong, but only because the
  wrong instantiation's body ran.
- It disappears when the dispatcher takes any additional discriminating NTTP, when only
  one of the siblings is dispatched, or when a *specialization* reflection (a
  declaration, which mangles with its full signature) is passed instead of the template
  reflection — which is why shallow/direct-call bisects all look fine.

## Suggested fix (cherry-pickable)

`Cfretz244/llvm-project @ b329d544cb20`: in `mangleReflection`'s
`ReflectionKind::Template` case, for `FunctionTemplateDecl`s only, append a
`$`-bracketed ODR hash (`ODRHash::AddTemplateParameterList` +
`AddFunctionDecl(pattern, /*SkipBody=*/true)`) after the template name. ODR hashes are
cross-TU-stable (modules rely on this), preserving legitimate linkonce_odr merging.

We first tried mangling the template head (`mangleTemplateParamDecl` per parameter +
requires-clauses) plus the pattern's function type; that is more in the spirit of the
Itanium scheme but asserts on real-world dependent pattern types —
`mangleFunctionParam`'s `parmDepth < FunctionTypeDepth.getDepth()` fires for
parameter-referencing expressions (Abseil's lifetimebound SFINAE / `noexcept(...)`)
mangled outside a function-declaration context. If a structural encoding is preferred
upstream, that assertion path needs handling first.

Regression test included:
`libcxx/test/std/experimental/reflection/substitute-nested-dependent.pass.cpp` — runtime
assertions (distinct addresses + per-body results); `static_assert`s cannot catch the
fold because the AST is always correct.

## Possibly related

The same name-only philosophy appears in other `mangleReflection` cases; we have a
separate open observation (qualifier predicates through entity proxies of
`absl::StatusOr<int>::value()`) that is NOT fixed by this change and will be reported
separately once minimized.
