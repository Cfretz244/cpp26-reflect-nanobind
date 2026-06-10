# TC-0007 — members_of instantiates member function bodies when it triggers the class's instantiation

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project. `SemaMetaActions::EnsureInstantiated` — the single completion
  funnel for every type-completing metafunction — completed specializations
  with `TSK_ExplicitInstantiationDefinition` PLUS
  `InstantiateClassTemplateSpecializationMembers` (every member body,
  explicit-instantiation-definition semantics). Fix mirrors
  `Sema::RequireCompleteTypeImpl`: `TSK_ImplicitInstantiation`, no member
  sweep, plus the member-class-of-a-template branch the sweep used to cover
  as a side effect. Regression test
  `members-of-lazily-ill-formed-bodies.pass.cpp`. Upstream:
  `repros/TC-0007/UPSTREAM.md`.
- **Track:** toolchain (clang-p2996, pinned by this repo; same family as the
  bloomberg fork base `837da39eb88c`)
- **Found via:** `corpus/runs/expected` (TartanLlama/expected v1.3.1). A bare
  `members_of(^^tl::expected<void, std::string>, access_context::unchecked())`
  loop — no binder, no substitution, just enumeration — produces 9 hard errors
  out of tl's implementation details.
- **Repro:** `corpus/findings/repros/TC-0007/repro.cpp` (standalone, no nanobind,
  no library; `-fsyntax-only`; `-DPREINSTANTIATE` shows the control).

## Symptom

When a `std::meta::members_of` call is the FIRST thing to instantiate a class
template specialization, clang-p2996 instantiates the members' *definitions*,
not just their declarations. A specialization whose member bodies are
lazily-ill-formed — valid C++ as long as those members are never odr-used, the
entire "specialized storage base" idiom — is wrong-rejected:

```cpp
template <class T> struct storage { T m_val; };
template <> struct storage<void> { char m_dummy; };

template <class T> struct Exp : private storage<T> {
  T* valptr() { return &this->m_val; }  // body valid for every T except void
};

consteval int n(std::meta::info c) {
  int k = 0;
  for (auto m : std::meta::members_of(c, std::meta::access_context::unchecked())) ++k;
  return k;
}
static_assert(n(^^Exp<void>) > 0);
// error: no member named 'm_val' in 'Exp<void>'
// note: in instantiation of member function 'Exp<void>::valptr' requested here
//   (from <meta>'s member-range iterator m_next)
```

The smoking-gun inconsistency: adding one ordinary use *before* the probe —
`Exp<void> ok_instance;` — makes the IDENTICAL `members_of` loop compile clean
(the class got completed with normal lazy semantics first, and enumeration of
an already-instantiated class does not re-instantiate). Whether reflection
succeeds on a type may not depend on whether unrelated earlier code happened to
instantiate it. It looks like the reflection path completes the class with
explicit-instantiation-definition-like eagerness instead of plain implicit
instantiation.

## Field shape

`tl::expected<void, std::string>`: the primary template's private
`valptr()`/`swap_where_both_have_value`/`swap_where_only_one_has_value_and_t_is_not_void`
helpers reference `this->m_val` and call the `enable_if`'d `val()` — valid for
every `T` except `void`, never instantiated for `void` by ordinary use (tl ships
and tests `expected<void, E>` since 2017). Bare enumeration yields:

```
tl/expected.hpp:1292: error: no member named 'm_val' in 'tl::expected<void, std::string>'   (valptr)
tl/expected.hpp:1882: error: no matching member function for call to 'val'                  (swap helpers)
... (9 errors total)
```

so the reflection-driven binder cannot even LOOK at the void specialization.
Pre-instantiating the spec dodges THIS finding but not TC-0006 (the
`can_substitute` SEGV on the same class's `swap` member template), so
`corpus/runs/expected` drops `expected<void, std::string>` entirely.

## Relationship to TC-0006

Independent bugs that co-occur on the same field type: TC-0007 is over-eager
*definition instantiation during enumeration* (wrong rejection, order-dependent,
dodged by pre-instantiation); TC-0006 is a *crash in substitution-failure
handling* (order-independent, not dodged). The repros are disjoint.
