# BINDER-0013 — copy constructors were never bound: Python could not copy a bound instance

- **Status:** FIXED (binder) — `nanobind/include/nanobind/nb_reflect.h`,
  `bind_class_contents`; regression test `test40_copy_construction`
- **Found via:** `corpus/runs/expected` (Tier 3, TartanLlama/expected v1.3.1).
  The run's `test_copy_ctor_differential` (`ExpInt(ok)` mirroring native
  `EI copy(ok)`) failed with `TypeError: __init__(): incompatible function
  arguments` once the compile-blockers (TC-0008, BINDER-0012) landed — the
  subset rationale had always promised "default + copy ctors".
- **Files:** `nanobind/include/nanobind/nb_reflect.h` (constructor pass).

## Symptom

The constructor pass has always excluded copy and move constructors
(inherited from the original PoC), and no other path bound them — so no
reflected class could be copy-constructed from Python, even though copying a
bound instance is part of any copyable type's real API (the corpus binds the
REAL API, and tl::expected's differential suite exercises it).

## Root cause

`!std::meta::is_copy_constructor(fn) && !std::meta::is_move_constructor(fn)`
in the ctor-pass guard, with no compensating `init<const T&>` anywhere. The
move exclusion is correct (a bound `init<T&&>` would gut its Python source
object); the copy exclusion was simply never compensated.

## Fix

After the ctor pass, bind Python-side copy construction explicitly:

```cpp
if constexpr (!std::is_abstract_v<T> && !has_reflect_trampoline<T>
              && std::is_copy_constructible_v<T>)
    cls.def(init<const T&>());
```

`std::is_copy_constructible_v` is false for deleted/private copy ctors, so
the BINDER-0012 contract is preserved. Trampolined classes are excluded:
`nb::init` placement-news the Alias for Python-derived instances, and a
trampoline has no `(const T&)` constructor.

## Verification

- `test40_copy_construction` (`NoDefault(NoDefault(21))` → distinct object,
  same state); full reflection suite green.
- `corpus/runs/expected`: `test_copy_ctor_differential` and the void-spec
  copy differential pass; run at outcome E.
