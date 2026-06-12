# GCC-0005 — deferred dependent noexcept-specifier ICEs partial-spec matching / is_noexcept

- dedup_key: `gcc16-nothrow-spec-p-deferred-noexcept`
- compiler: g++ (GCC) 16.1.0 aarch64-linux-gnu (`gcc:16` docker image)
- status: workaround landed in the binder (`nb_fn_type_of`);
  **RESOLVED UPSTREAM, no filing needed** — fixed on trunk by
  `05ea83ffd54` (PR c++/124628, Patrick Palka, 2026-05-14: reflection
  queries now mark_used the function, instantiating deferred noexcept /
  doing return-type deduction first), bisect-verified in the devenv
  (probe ICEs at the parent, compiles at the commit). Already
  cherry-picked to releases/gcc-16 as `e1396e44961`
  (releases/gcc-16.1.0-90) ⇒ ships in GCC 16.2. Retire the binder shim
  after the container moves to 16.2 and the probe flips. Details:
  `corpus/findings/repros/GCC-0005/UPSTREAM.md`.
- found by: corpus run `json` (first GCC-native ICE at corpus scale)

## Symptom

`internal compiler error: in nothrow_spec_p, at cp/except.cc:1240`, with a
backtrace through `most_specialized_partial_spec` →
`instantiate_class_template` (binder matrix dispatch) or directly from
`std::meta::is_noexcept` on a function TYPE (the emit backend's
`spell_fn_tail`).

## Root cause shape

A member function of a class template declared with a DEPENDENT
noexcept-specifier — `void swap(reference) noexcept(is_nothrow_...<T>::value)`
(nlohmann `basic_json<>::swap` is the field case) — keeps its
exception-specification DEFERRED in the instantiated member until something
forces resolution. Two consumers then hit the `nothrow_spec_p` assert:

1. Matching the spliced function type `[: type_of(^^T::swap) :]` against a
   partial-specialization matrix keyed on `Ret(Args...) [noexcept]`.
2. Calling `std::meta::is_noexcept` on the function TYPE reflection.

`std::meta::is_noexcept` on the function DECLARATION resolves the specifier
as a side effect and is the workaround.

## Repro

`gcc16-proveout/probes/xfail_gcc5_deferred_noexcept_partial_spec.cpp`
(self-contained, ~70 lines):

```
g++ -std=c++26 -freflection xfail_gcc5_deferred_noexcept_partial_spec.cpp
```

Expected: compiles, runs, exits 0 (clang-p2996 behavior). Actual: the ICE.

## Binder workaround (landed, nanobind mk-reflect 58543ff)

`nanobind::detail::nb_fn_type_of(fn)` calls `is_noexcept(fn)` (forcing
resolution) before returning `type_of(fn)`; every decl-derived function-type
splice (method/static/free/operator/member-template dispatch in
`nb_reflect.h`, the four `type_of(Fn)` sites feeding emit-lane signature
spelling in `nb_reflect_emit.h`) routes through it. Behavior-neutral on
clang-p2996 (both suites 129/129). Unit regression: `Box<T>::swap_with`
(dependent noexcept) in the shared fixture.

Any NEW code that splices a declaration's function type into a template
match must use the helper.
