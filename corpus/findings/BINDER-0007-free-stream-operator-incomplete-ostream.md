# BINDER-0007 — Free `operator<<(std::ostream&, T)` bound as a dunder → incomplete-ostream error

- **Status:** **RESOLVED as a feature** (stream insertion → Python `__str__`) — pinned
  `nanobind @ 58c818a`.
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

## Fix (implemented, nanobind @ 58c818a)

Rather than skip, the binder now turns this into a feature. `is_stream_type` /
`involves_stream_type` detect a stream operand (after `dealias`/`remove_cvref`, the type spelling
is `std::basic_ostream`/`basic_istream`); `is_bindable_free_operator` excludes such operators from
the dunder scan, **keyed on the operand type, not the operator symbol** (so the genuine
`operator<<(int128, int)` shift still maps to `__lshift__`). Separately, `bind_stream_str<T>`
binds **`__str__`** for any ostream-insertable `T`, formatting via `std::ostringstream` and
returning `std::string` — `ostream` never reaches Python. Stream extraction (`operator>>` /
`std::istream`) is skipped. Regression test: `stream_test::Streamable` (`str(s)` works; `s << 1` →
`__lshift__`). dedup_key: `free-stream-operator-as-dunder`.

## Result in the corpus

`absl::int128`/`uint128` now bind (`corpus/runs/abseil_numeric`, outcome E) with full
arithmetic/bitwise/comparison dunders and a working `str()`; likewise `str(absl::Duration)` →
`"1m30s"` and `str(absl::Status)`. Linking is via the prebuilt absl static lib (`link_abseil`),
not per-`.cc` `extra_sources`.
