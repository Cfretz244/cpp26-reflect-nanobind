# TC-0004 — substitute() results misreport predicates under nested-dependent instantiation

- **Status:** OPEN (toolchain; worked around in the binder). Candidate for minimization +
  upstreaming to bloomberg/clang-p2996 — alongside TC-0002 this is the second-most
  load-bearing toolchain bug the prove-out has hit.
- **Found via:** Wave-4 container binding. `absl::flat_hash_map<int,std::string>` bound with
  the full member-template surface (`contains`/`find`/`at`/`count`/`erase`) **except**
  `operator[]` — silently absent, no diagnostic.
- **Triage:** `toolchain`. dedup_key: `nested-dependent-substitute-misreport`.

## Symptom

A member function template's default instantiation, obtained via
`std::meta::substitute(tmpl, {})`, answers kind/qualifier predicates
(`is_operator_function`, `is_volatile`, `is_rvalue_reference_qualified`) **incorrectly —
silently — when the substitute() call is evaluated two or more template levels deep**
(a function template instantiated from a `template for` body inside another dependent
function template). The same calls evaluate correctly:

- at non-dependent scope (verified),
- one dependent level down (a `template for` over `members_of(^^T)` directly in a
  function template — verified),
- when the SAME substituted reflection is computed at a shallower level and passed down
  as a template argument (the binder's workaround).

## Repro skeleton (the binder shape; B mirrors absl's raw_hash_map operator[] cluster)

```cpp
template <bool C> using EnableIf = std::enable_if_t<C, int>;
template <class K, bool V> inline constexpr bool LTB = !V;

struct B {
    // instantiable variant (default instantiation exists)
    template <class K = int, class P = double, int = EnableIf<LTB<K, false>>()>
    std::string& operator[](K&& k);
    // non-instantiable pack sibling (SFINAE-false; like absl's lifetimebound pair)
    template <class K = int, class P = double, int&..., EnableIf<LTB<K, true>> = 0>
    std::string& operator[](K&& k);
};

template <typename T> void level1(auto& cls) {              // dependent level 1
    template for (constexpr auto fn : std::define_static_array(
            std::meta::members_of(^^T, std::meta::access_context::unchecked()))) {
        if constexpr (std::meta::is_function_template(fn))
            level2<T, fn>(cls);                              // dependent level 2
    }
}
template <typename T, std::meta::info tmpl> void level2(auto& cls) {
    if constexpr (can_substitute(tmpl, {})) {                // true (correct)
        constexpr auto spec = std::meta::substitute(tmpl, std::vector<std::meta::info>{});
        // HERE: is_operator_function(spec) silently returns false (should be true),
        // and in an earlier binder iteration is_volatile/is_rvalue_reference_qualified
        // misreported as well -- the operator never binds, no diagnostic.
    }
}
```

Bisect evidence (all verified on the pinned toolchain):
- direct `reflect_bind_member_template<B, tmpl>(cls)` with a consteval-computed NTTP: binds.
- the same call from a dependent `template for` (level-2 substitute): silently nothing.
- substitute at level 1 + pass `spec` as an NTTP to level 2: binds.
- the failure requires the **pack-sibling overload** to be present (a lone instantiable
  template binds even at level 2) — overload-set resolution during nested instantiation
  appears implicated.

## Related (same family): qualifier misreport through entity proxies

`is_rvalue_reference_qualified(underlying_entity_of(proxy))` returns false for all four
`StatusOr<int>::value()` overloads (incl. the `&&`-qualified ones) — recorded in the
TC-0003 addendum. Both look like the substituted/unwrapped declaration not being fully
resolved when produced inside template instantiation.

## Workaround in the binder (nanobind @ the pin carrying this finding)

1. `substitute(fn, {})` is performed **at the dispatch-loop level** (level 1) in
   `bind_class_contents` / `flatten_base_members`, and the resulting spec is passed to
   `reflect_bind_member_template<T, tmpl, spec>` as a frozen NTTP.
2. Qualifier filtering never trusts decl predicates on substituted specs: the gate is
   "does a binder partial specialization exist for this exact function type"
   (`requires { sizeof(reflect_method_binder<T, fn, FnType>); }` — the undefined primary
   is a substitution failure), in `reflect_bind_operator`, the member-template named
   path, and `reflect_bind_proxy`.

## Next step for upstreaming

Minimize to a standalone repro with no nanobind (the skeleton above, plus prints of the
misreported predicates at each nesting level) and file against bloomberg/clang-p2996.
