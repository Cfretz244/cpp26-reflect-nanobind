# BINDER-0010 — by-value move-only parameters hard-error the binding TU

- **Status:** FIXED (binder commit alongside the abseil_strings run)
- **Found via:** `corpus/runs/abseil_strings`. Binding `absl::Cord` head-on failed at
  Gate 4 with `error: call to deleted constructor of 'absl::CordBuffer'` out of
  `nb_func.h:281` — `Cord::Append(CordBuffer buffer)` takes the **move-only**
  `CordBuffer` **by value**.
- **Files:** `nanobind/include/nanobind/nb_reflect.h`.

## Root cause

nanobind's generic class caster produces a by-value argument by **copying** out of the
caster's storage (`operator cast_t<T>()`), so instantiating the dispatch lambda for any
function with a by-value parameter of a non-copy-constructible class type is a hard
compile error — one such overload anywhere in a bound class breaks the whole TU.

## Fix

`has_move_only_by_value_param(fn)` (consteval): true when any parameter is a
non-reference, non-pointer class type that is not copy-constructible
(`std::meta::is_copy_constructible_type`). Gated at all four binding paths — member
functions (`reflect_bind_member_function`, which also covers operators/conversions/
statics and the flattening pass), constructors (`reflect_bind_ctor`), free functions
(`reflect_free_function`), and free operators (`is_bindable_free_operator`). Sibling
overloads still bind (Cord keeps `Append(const Cord&)` / `Append(string_view)`).

Mirrors the earlier non-copy-assignable-member → `def_ro` fallback: graceful surface
reduction instead of a TU-wide hard error. Test: `Sink::put(MoveOnlyBuf)` skipped while
`put(int)` binds (`test27c_move_only_by_value_param_skipped`).

dedup_key: `move-only-by-value-param`.
