# BINDER-0006 — Zero-length C-array data member (absl::FixedArray internal storage) hard-errors

- **Status:** OPEN — worked around by excluding the type from the Abseil subset (not yet fixed)
- **Found via:** special-case Abseil run, `^^absl::FixedArray<int>` (outcome B while scoping)
- **File:** `nanobind/include/nanobind/nb_reflect.h`, data-member binding (`reflect_bind_member` /
  `with_data_extras` → `def_rw`) and `check_stl_casters`

## Symptom

`nb::reflect_<^^absl::FixedArray<int>>(m)` fails to compile. `FixedArray<int>` holds its inline
storage in an internal nested type the binder transitively discovers and tries to bind,
`absl::FixedArray<int>::StorageElementWrapper<int>`, whose data member is a **zero-length C array**
`int array[0]`:

```
nb_class.h:714: error: array type 'int[0]' is not assignable
   [p](T &c, Q value) { c.*p = (Q) value; },        // def_rw setter lambda
... in instantiation of class_<...StorageElementWrapper<int>>::def_rw<..., int[0]>
```

A second, related error comes from `check_stl_casters` forcing instantiation of FixedArray's
`AsValueType`, which in `fixed_array.h:431` does `return std::addressof(ptr->array)` — returning
`int*` from an `int(*)[0]`, ill-formed for the zero-length array.

## Root cause

Two intertwined gaps:
1. **Array-typed data members can't go through `def_rw`.** The setter `c.*p = value` requires an
   *assignable* member; a C array member (`int[0]`, and any `T[N]`) is not assignable. The binder
   already skips unnamed/volatile/template members but not array-typed ones.
2. **The binder recurses into a library-internal nested storage type.** `StorageElementWrapper` is
   an implementation detail reached only via FixedArray's own signatures; binding it is never
   desired.

## Fix sketch (not yet implemented)

Guard data-member binding to **skip members of array type** (`type_of(mem)` is an array) — they
have no `def_rw`-able form; expose via a property accessor later if ever wanted. That alone makes
`FixedArray` bind its public surface. Independently, consider not transitively binding nested types
that live in a library `detail`/internal scope. dedup_key: `array-data-member-def_rw`.

## Workaround in the corpus

`absl::FixedArray` is excluded from the Abseil run's `reflect_args`; `absl::InlinedVector<T,N>`
(which stores via a union, not a public array member) binds cleanly and carries the run to E.
