# BINDER-0007 — Free `operator<<(std::ostream&, T)` bound as a dunder → incomplete-ostream error

- **Status:** OPEN — worked around by excluding the type from the Abseil subset (not yet fixed)
- **Found via:** special-case Abseil run, `^^absl::int128` (outcome B while scoping)
- **File:** `nanobind/include/nanobind/nb_reflect.h`, `bind_free_operators` /
  `reflect_free_operator_binder` (the `NB_REFLECT_DEFINE_FREE_OP_BINDER` family)

## Symptom

`nb::reflect_<^^absl::int128>(m)` fails to compile:

```
__type_traits/is_base_of.h:26: error: implicit instantiation of undefined template
                                       'std::basic_ostream<char>'
  ... in instantiation of std::is_base_of_v<nanobind::detail::api_tag, std::basic_ostream<char>>
  ... type_caster<std::basic_ostream<char>>
  ... reflect_free_operator_binder<absl::int128, ^^operator<<,
                                   std::basic_ostream<char>&(std::basic_ostream<char>&, absl::int128)>
  ... bind_free_operators<absl::int128>
```

`absl::int128` has a namespace-scope `operator<<(std::ostream&, int128)`. When the binder binds a
class, `bind_free_operators` scans the enclosing namespace for free operators involving the type and
binds them as Python dunders (this is how `2.0 * vec` etc. work). For `operator<<` it tries to make
a caster for the *other* operand, `std::basic_ostream<char>` — which is only forward-declared
(`<iosfwd>`) at that point, so `type_caster<std::ostream>` instantiation hard-errors.

## Root cause

The free-operator scan does not exclude **stream insertion/extraction** operators. `operator<<` /
`operator>>` against `std::basic_ostream` / `std::basic_istream` are C++ I/O streaming, not Python
operators: an `ostream` is not a bindable Python type, so there is nothing meaningful to bind, and
even attempting it forces an incomplete-type instantiation. (`__lshift__` on a Python `int128`
taking a stream is also semantically wrong.)

## Fix sketch (not yet implemented)

In the free-operator scan, **skip `operator<<`/`operator>>` whose non-`T` operand is a
`std::basic_ostream`/`std::basic_istream` (or, more generally, whose other operand has no usable
caster).** Detect via the operand type's template (`std::basic_ostream`/`basic_istream`) and drop
those operators before instantiation. dedup_key: `free-stream-operator-as-dunder`.

## Workaround in the corpus

`absl::int128` is excluded from the Abseil run's `reflect_args`. (Once fixed, int128's arithmetic
and comparison operators are an excellent dunder-mapping target — but note int128 division and
streaming are `.cc`-defined, so it would also need an `extra_sources` link like InlinedVector.)
