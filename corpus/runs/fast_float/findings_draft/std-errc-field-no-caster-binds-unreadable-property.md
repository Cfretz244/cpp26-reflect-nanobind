dedup_key: binder-binds-data-member-of-uncastable-enum-type-as-unreadable-property

# Binder binds a data member whose type has no caster, yielding a property that raises only at access time

Layer: BINDER (low confidence -- arguably working-as-intended)

## Context
fast_float v8.2.8. `fast_float::from_chars_result_t<char>` is a 2-field aggregate:

```cpp
template <typename UC> struct from_chars_result_t {
  UC const *ptr;
  std::errc ec;              // <-- scoped enum, NO nanobind caster
  constexpr explicit operator bool() const noexcept { return ec == std::errc(); }
};
```

Reflected head-on via `nb::reflect_<^^fast_float::from_chars_result_t<char>>(m)`. The binder
emits a read accessor for every public data member, including `ec`. `std::errc` is a libc++
`enum class` that is NOT in the reflect set and has no registered type caster, so the
generated getter `(self) -> std::__1::errc` has no return conversion.

## Smallest trigger
```cpp
struct R { int* ptr; std::errc ec; };
// nb::reflect_<^^R>(m);   // in a module
```
```python
r = mod.R()
r.ptr            # -> None        (ok: int* has a caster)
bool(r)          # -> fine if operator bool present
r.ec             # -> TypeError: Unable to convert function return value to a Python type!
                 #    The signature was (self) -> std::__1::errc
```

## First diagnostics
Compiles and imports cleanly; the failure is deferred to attribute-access time:

    TypeError: Unable to convert function return value to a Python type!
    The signature was (self) -> std::__1::errc

## Why this is (only) a suspected finding
This matches nanobind's general "no caster => runtime TypeError" model, so it is plausibly
working-as-intended rather than a defect. The note is filed because a field that can NEVER be
read (its type has no caster and is not in the bind set) is a candidate for the same graceful
skip the binder already applies to member/constructor templates (BINDER-0001) and deleted
functions (BINDER-0012): binding an unreadable property is strictly worse than omitting it,
since it advertises surface that always throws. A targeted alternative would be to also pull
a member's scoped-enum type into the enum-binding pass (std::errc would then bind as an enum).

## Run impact
None on the outcome: the run reaches E. The fixture wrappers surface the errc as a plain int
(`static_cast<int>(r.ec)`) so the error codes are still differentially checked; the head-on
`from_chars_result_t<char>` binding asserts ptr + operator bool work and that `.ec` raises
(locked in, not hidden). Recorded in skipped_features.
