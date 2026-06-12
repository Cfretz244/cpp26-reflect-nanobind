# GCC-0006 — define_static_array lift instantiates a constexpr member's body

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

## Second site — the eigen run (member-level exclude_ collision)

The `eigen` run (BINDER-0014 flagship) is a SECOND, harder site of this same
divergence. Eigen's vector-only accessors (`w()`/`x()`/`y()`/`z()`/`operator[]`)
and vector-`resize()` are `constexpr` and `static_assert` a shape in their
bodies — exactly the lazily-ill-formed-constexpr-member shape — and they are
NOT `enable_if`-ed away (unlike expected's `value()`/`operator*`), so they are
present in `members_of` for the wrong-shaped bound spec. Two consequences:

1. The BINDER-0014 per-member `nb::exclude_<...>` escape hatch works by lifting
   each member's REFLECTION into the marker, which is itself a materialization —
   so on GCC, forming the exclusion instantiates the very bodies it means to
   exclude. The reflection-NTTP exclusion route is GCC-incompatible for these
   members; `never_bound_plain_member_fn` does NOT cover them (they are mapped,
   public, ordinary methods/operators).
2. The binder's base-flatten lift instantiates them anyway (the facade bases are
   bound as real Python bases).

Resolved by a new GCC-safe binder capability, `nb::exclude_member_<Owner,
"name">` — exclude a member by NAME, dropping it before the lift without ever
forming its reflection (owner is a class, safe to materialize; name is a
fixed_string). See `corpus/runs/eigen/findings_draft/BINDER-DRAFT-1-exclude-member-by-name.md`
(dedup_key gcc6-exclude-member-by-name). This is the general companion to
`never_bound_plain_member_fn`: where the latter drops members no pass consumes,
the former drops a mapped member the binding author marks by name.

## Status

Worked around in the binder — see BINDER-DRAFT-1
(`never_bound_plain_member_fn` drops never-bound members before the lift) and
the eigen run's BINDER-DRAFT-1 (`nb::exclude_member_` by-name drop for mapped
members the author excludes).

**PATCH READY** (Phase 4, 2026-06-12): minimized, root-caused, and fixed on
the devenv trunk tree, jointly with GCC-1 (same root cause): the P0859
pre-pass `instantiate_constexpr_fns` / `instantiate_cx_fn_r`
(gcc/cp/constexpr.cc) walks into `REFLECT_EXPR` operands and
instantiates/synthesizes every reflected constexpr/defaulted member —
but forming a reflection is not a [basic.def.odr] naming; only splicing
through it is, and those paths mark-use on their own. Fix: don't walk into
`REFLECT_EXPR` subtrees. Verified: both xfail probes (gcc1, gcc6) compile
AND run; testsuite slices 3939 passes / 0 unexpected fails; regression test
g++.dg/reflect/members-of-lift1.C added. Patch + bugzilla material:
`corpus/findings/repros/GCC-0006/` (UPSTREAM.md). Not yet filed — filing is
the user's call.
