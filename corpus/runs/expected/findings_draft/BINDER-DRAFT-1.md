# BINDER-DRAFT-1 — never_bound_plain_member_fn: keep lazily-ill-formed constexpr members out of the lift (GCC)

- **dedup_key**: gcc-constexpr-member-body-lift-instantiation
- **run**: expected (TartanLlama/expected v1.3.1)
- **backend**: gcc16
- **related GCC finding**: GCC-DRAFT-1 (dedup_key gcc6-constexpr-member-lift),
  probe `gcc16-proveout/probes/xfail_gcc6_constexpr_member_lift.cpp`

## Symptom

`expected` was green on clang (E/E, surface pass) but outcome **B (B.compile)**
on GCC 16 at gate 4. The compile error was in the library header, not the
binder:

```
tl/expected.hpp:1293: error: 'const class tl::expected<void, std::string>'
  has no member named 'm_val'; did you mean 'val'?
  const T *valptr() const { return std::addressof(this->m_val); }
... required from 'constexpr const T* tl::expected<T,E>::operator->() const
    [with T = void; E = std::string]'
... required from meta:658  reflect_constant_array(__r)   (i.e. the
    define_static_array lift in bind_class_contents, T = expected<void,string>)
```

## Root cause

GCC-side divergence (NOT a binder logic bug): lifting the reflection of a
`constexpr`/consteval member function into `std::define_static_array`
(`reflect_constant_array`) **instantiates that member's definition** — GCC must
decide whether the function is usable in constant evaluation to materialize its
reflection as an NTTP, and that decision instantiates the body. For
`tl::expected<void, std::string>`, the unconditionally-declared
`constexpr operator->() const` (-> `valptr()` -> `addressof(this->m_val)`) is
lazily ill-formed for the void storage base (no `m_val`), so the lift itself is
a hard error — even though the binder never binds `operator->`.

Minimized to two members with the identical ill-formed body, one `constexpr`
and one not: only the constexpr one is instantiated by the lift. clang-p2996
never instantiates a body on lift, so it binds the void spec cleanly. This is
the same family as catalog GCC-1 (implicit special members) and GCC-2
(eager-instantiation-on-lift), generalized to ANY constexpr member with a
lazily-ill-formed body. Repro: `xfail_gcc6_constexpr_member_lift.cpp`.

## Fix summary

New consteval predicate `never_bound_plain_member_fn(info)` and a GCC-only
filter in `liftable_class_members` (nb_reflect.h). It drops, BEFORE the lift,
member functions that no binding pass can ever consume:

- non-public functions (e.g. expected's private `valptr`/`errptr`), and
- public member operators with no Python dunder mapping
  (`operator->`, unary `operator*`, address-of, prefix `++/--`, `<=>`, …),
  via the existing `operator_dunder(operator_of(fn), arity)` classifier.

These are exactly the members the binder already skips downstream, so the bound
surface is unchanged on both backends; the change only keeps their
lazily-ill-formed constexpr bodies out of static storage. The decision lives in
the shared classifier (nb_reflect.h), guarded `#if !defined(__clang__)` (inert
on clang). It does NOT (and cannot, without instantiating) drop a *mapped* or
ordinary member whose constexpr body is lazily ill-formed; no such member arises
in practice because libraries `enable_if` those away (so they are absent from
`members_of` for the offending spec — true for expected's `value()`/`operator*`).

## Files touched

- `nanobind/include/nanobind/nb_reflect.h` — added `never_bound_plain_member_fn`
  and called it from the GCC-only branch of `liftable_class_members`.
- `gcc16-proveout/probes/xfail_gcc6_constexpr_member_lift.cpp` — GCC bug repro.

## Validation

- expected run: `outcome=E constexpr=E emit=E surface=pass` (matches clang
  result.json: E/E, 16 passed, surface pass).
- Unit suites after the edit: GCC container 129 passed; clang host 129 passed.
- No surface change: the dropped members were never bound on either backend.
