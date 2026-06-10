dedup_key: binder-nonowning-view-ctor-from-pystr-dangles
layer: BINDER
smallest_trigger: bind a class C with a C(const std::string&) ctor that stores a pointer into the string; construct C("x") from Python and later read it

# Binding a non-owning view class (ctor takes std::string, stores a pointer into it) yields a dangling Python object

## Summary

`leveldb::Slice` is a non-owning view: `Slice(const std::string& s) : data_(s.data()), size_(s.size())`
just captures the string's buffer pointer (the library documents this -- "the user must ensure
the slice is not used after the corresponding external storage has been deallocated").

When `Slice` is bound head-on and constructed from Python:

```python
s = m.Slice("hello")     # nanobind makes a temporary std::string from "hello",
                         # calls Slice(const std::string&), then FREES the temporary
s.ToString()             # reads data_[0..size_] -> dangling / garbage
s.size()                 # 5 (size_ is stored by value -> still correct)
```

The Python `Slice` object outlives the `std::string` temporary nanobind created for the
constructor argument, so `data_` dangles immediately after `__init__` returns. Methods that only
read by-value state (`size()`, `empty()`) are fine; any method that dereferences `data_`
(`ToString()`, `compare()`, `starts_with()`, `operator[]`) reads freed memory.

This also breaks passing such a held `Slice` into a later call: e.g. `db.Put(opts, key_slice,
val_slice)` stores garbage because the slices dangled before the call even began.

## Why it matters / scope

- This is the general "view type" hazard for any reflected ctor whose parameter is a temporary
  the binder frees, where the constructed object retains a pointer/reference into that temporary.
  It is not unique to leveldb: `std::string_view`, `absl::string_view`, span-like and
  `*_view`/`Slice`/`Ref` types all have this shape.
- The binder cannot, in general, know a ctor retains a pointer into its argument. But a possible
  mitigation worth considering: for a bound constructor, keep the converted C++ argument
  temporaries alive for the *lifetime of the constructed Python object* when the target type is
  trivially-copyable-with-pointer-members / flagged as a view (an opt-in keep_alive-on-construct,
  analogous to nanobind's keep_alive for methods). Today there is no annotation that expresses
  "this ctor's argument must outlive the result".
- At minimum this deserves documentation: head-on binding of non-owning view types is unsafe
  when the view is constructed from a Python-owned/temporary buffer.

## Workaround used in this run

Slice is still bound head-on, but its pointer-dereferencing surface is exercised only where the
backing bytes are guaranteed live (constructed and consumed entirely inside C++ via the
KVStore/batch fronts, which take std::string -- kept alive by the string caster for the call --
and build a live Slice internally). The Python-level Slice tests assert only the by-value state
(size/empty) that does not dereference the dangling pointer, plus presence of the pointer-deref
methods (Layer 3).
