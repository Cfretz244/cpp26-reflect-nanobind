dedup_key: lvalue-ref-class-return-defaults-to-copy-policy

# Method returning `T&` (lvalue reference to a registered class) defaults to rv_policy::copy -> runtime abort on noncopyable T

Layer: BINDER

## Summary

`effective_rv_policy` / `returns_raw_class_pointer` in `nb_reflect.h` give a borrowing
policy (`reference_internal` on methods, `reference` on statics/free fns) ONLY to returns
that are raw class *pointers* (`T*`). A method returning a raw lvalue *reference* to a
registered class (`T&`) is NOT matched: `returns_raw_class_pointer` requires
`is_pointer_type`, so `effective_rv_policy` falls through to `rv_policy::automatic`. For a
non-pointer, non-smart-pointer return nanobind's `automatic` resolves to `rv_policy::copy`.

Consequences for a method `T& f()`:
- T copyable: nanobind copy-constructs an owned Python object. No crash, but the Python
  object is a DETACHED COPY, not a view of the C++ object -- mutations and identity are
  silently wrong (same class of double-ownership/aliasing bug that motivated BINDER-0017 for
  `T*`, just unaddressed for `T&`).
- T NONCOPYABLE: `nb_type_put_common(..., rv_policy::copy, ...)` finds no copy function for
  the type and calls `detail::fail_unspecified()` -> `abort()`. The user sees only
  "Critical nanobind error: encountered an unrecoverable error condition" with NO indication
  of which call or why. The binding COMPILES and IMPORTS cleanly; the abort is at the first
  call of the reference-returning method.

`T&` returns of registered classes are extremely common in real APIs: fluent builders that
return `*this` as `T&` (not `T*`), container/owner accessors that hand out an internal
object by reference, etc. Taskflow is the field shape: a fixture method returning
`tf::Taskflow&` (the run's natural way to hand Python the real graph container) aborts,
because `tf::Taskflow` is move-only (copy ctor deleted). The same gap would hit any
head-on `T& method()` on the library's own classes.

## Smallest trigger

A registered, non-copy-constructible class T and any bound method/function returning `T&`:

```cpp
struct Noncopyable {              // registered via reflect_
  Noncopyable() = default;
  Noncopyable(const Noncopyable&) = delete;
  Noncopyable(Noncopyable&&) = default;
  int v = 7;
};
struct Holder {
  Noncopyable n;
  Noncopyable& get() { return n; }   // <-- bound with rv_policy::copy
};
// reflect_<^^Holder, ^^Noncopyable>(m);
// Python: Holder().get()   ->  abort ("Critical nanobind error ...")
```

For a copyable T the same `get()` returns a wrong detached copy instead of a view.

## First diagnostics

Runtime abort (compiles + imports fine). lldb backtrace at the abort:

```
abort
nanobind::detail::fail(char const*, ...)
nanobind::detail::fail_unspecified()
nanobind::detail::nb_type_put_common(void*, type_data*, rv_policy, cleanup_list*, bool*)
nanobind::detail::type_caster_base<tf::Taskflow>::from_cpp<tf::Taskflow&>(value, policy=copy, cleanup)   <-- policy=copy
   ... reflect_method_binder<tftest::Diamond, &Diamond::taskflow> ...
```

The `policy=copy` argument is the tell: the binder handed `automatic` to nanobind for a
`T&` return and nanobind resolved it to `copy`.

## Suggested direction (not applied -- binder is off-limits here)

Extend the borrow-default reasoning (BINDER-0017) from `T*` to lvalue-reference-to-class
returns: a function returning `T&` to a registered class should default to
`reference_internal` (methods) / `reference` (statics/free fns), exactly like `T*`. A
return BY VALUE (`T`) must keep `automatic`/copy; only the reference case is wrong.
`returns_raw_class_pointer` could grow a sibling `returns_lvalue_ref_to_class` and
`effective_rv_policy` treat both the same.

## Run-side workaround

The taskflow run's fixture returns `tf::Taskflow*` (pointer) instead of `tf::Taskflow&`,
which the binder already borrows correctly (`reference_internal`). All head-on querying of
the real `tf::Taskflow` then works.
