# GCC-DRAFT-1 — define_static_array lift instantiates a constexpr member's body

- **dedup_key**: gcc6-constexpr-member-lift
- **GCC number**: GCC-6 (next after the existing GCC-1/2/3/5 probes)
- **probe**: `gcc16-proveout/probes/xfail_gcc6_constexpr_member_lift.cpp`
- **component**: c++ (reflection / constant evaluation)
- **GCC**: 16.1.0 (gcc16-reflect container)

## Summary

Lifting the reflection of a `constexpr` (or consteval) member function into
`std::define_static_array` / `std::meta::reflect_constant_array` — i.e.
materializing the member's `std::meta::info` as an NTTP — **instantiates that
member's definition**. When the member's body is lazily ill-formed in the
specialization being reflected, the lift is a hard error, even though the
member is never odr-used.

A non-constexpr member with the byte-identical ill-formed body is NOT
instantiated by the same lift, so `constexpr`-ness is the trigger: GCC appears
to instantiate the body to decide the function's usability in constant
evaluation when forming its reflection constant.

## Expected vs actual

```
$ g++ -std=c++26 -freflection -fsyntax-only xfail_gcc6_constexpr_member_lift.cpp
expected: clean — the lift materializes reflections only; bodies are untouched
          (clang-p2996 with -freflection-latest compiles the analogous program)
actual:   error: 'const struct S<void>' has no member named 'm_val'
          required from 'constexpr const T* S<T>::cefn() const [with T = void]'
          required from the define_static_array(members_of(...)) lift
```

`members_of(...)` alone (no lift) is fine — see the `#if 0` block in the probe.

## Field origin

`tl::expected<void, E>` (TartanLlama/expected v1.3.1). The unconditionally
declared `constexpr operator->() const` returns `valptr()` =
`addressof(this->m_val)`; the void storage base has no `m_val`. Reflecting the
`expected<void, std::string>` specialization to generate Python bindings lifted
its member list into `define_static_array`, instantiating `operator->`'s body
and hard-erroring. clang-p2996 binds the same specialization with no error.

## Relationship to existing findings

Same family as GCC-1 (lift instantiates an implicit special member's
definition) and GCC-2 (eager instantiation on lift), generalized: any
`constexpr` member function with a lazily-ill-formed body bites, not only
implicit/special members.

## Status

Not patched in GCC (per discipline). Worked around in the binder — see
BINDER-DRAFT-1 (`never_bound_plain_member_fn` drops never-bound members before
the lift). To be filed on gcc.gnu.org bugzilla in Phase 4.
