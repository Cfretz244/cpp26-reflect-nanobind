# BINDER-0001 — Templated members hard-error instead of being gracefully skipped

- **Status:** FIXED (binder)
- **Found via:** Phase 0b, sgorsten/linalg `vec<float,3>` (outcome B → E after fix)
- **File:** `nanobind/include/nanobind/nb_reflect.h`, `bind_class_contents` + `flatten_base_members`

## Symptom

`nb::reflect_<^^linalg::vec<float,3>>(m)` failed to compile:

```
error: constexpr variable '__range' must be initialized by a constant expression
   std::define_static_array(std::meta::annotations_of(R))   // nb_reflect.h:72
note: cannot query the annotations of a constructor template
```

`linalg::vec<T,N>` has **member templates**: templated converting constructors
(`template<class U> vec(const vec<U,2>&)`, a `converter` constructor) and a templated
conversion operator. The member-iteration loops in `bind_class_contents` did not exclude
templates, so the binder reached a constructor/conversion *template* and called
`annotations_of()` / `is_property_getter<fn>()` on it — ill-formed for a template, hard error.

## Root cause

The binder's own convention (and `nanobind/CLAUDE.md`) is that unsupported function shapes
(`volatile`, `&&`, variadic) are *gracefully skipped*. Member function templates are documented
as unsupported but were **not** skipped — three loops in `bind_class_contents` (constructors,
methods, properties) and one in `flatten_base_members` lacked the `!std::meta::is_template(fn)`
guard that other loops in the same file already use (e.g. lines ~764, ~1050, ~1136).

## Fix

Add `!std::meta::is_template(fn)` to the constructor, method, and flatten-method `if constexpr`
conditions. For the **property** loop the guard must *nest*: `is_property_getter<fn>()` itself
instantiates `annotations_of(fn)`, so `&&` short-circuit is insufficient (the helper is still
instantiated). Restructured as an outer `if constexpr (... && !is_template)` enclosing the
`if constexpr (is_property_getter<fn>())`.

## Verification

- `linalg::vec<float,3>` now binds: data members, constructors, `operator[]`→`__getitem__`;
  differential + invariant tests pass (corpus/runs/linalg, outcome E).
- The existing reflection suite still passes (41/41) — no regression.

## Upstream

This is a binder (nanobind fork) fix, not a toolchain bug — lands on `mk-reflect`.
Note for hardening: `vec(const T* p)` binds as `__init__(self, p: float)` (pointer parameter
treated as scalar) — a separate suspected issue worth a dedicated finding/test.
