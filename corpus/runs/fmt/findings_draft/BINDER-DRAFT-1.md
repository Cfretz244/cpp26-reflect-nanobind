# BINDER-DRAFT-1 — builtin-type CamelCase spec names diverge clang vs GCC

- **dedup_key**: spec_camel_name_builtin_type_spelling_divergence
- **run**: fmt (slug `fmt`), gcc16 backend
- **lanes affected**: constexpr + emit (the shared classifier `spec_camel_name`)

## Symptom

fmt binds `fmt::to_string<long long>` (a free function-template instantiation listed
in the reflect pack). Its auto-generated Python name (BINDER-0003 CamelCase + enable_if
NTTP suffix) came out as `to_stringLonglongint0` under GCC 16 but `to_stringLonglong0`
under clang-p2996. The run's differential test `test_real_fmt_to_string_instantiations`
calls `m.to_stringLonglong0(...)`; under GCC that attribute was named `to_stringLonglongint0`,
so Gate 6 (correctness) failed on BOTH lanes (compile/import passed; value comparison via
the bound name failed). `to_stringInt0`/`to_stringDouble0` were unaffected (`int`/`double`
spell identically across compilers).

## Root cause

`spec_camel_name` (nb_reflect.h) builds the CamelCase name by appending each template
argument. A builtin scalar type carries no identifier, so it fell through to
`sanitize_identifier(display_string_of(arg))`. `display_string_of` spells the integral
builtins with the compiler's canonical wording, which DIFFERS:

| type                 | clang display      | GCC/libstdc++ display     |
|----------------------|--------------------|---------------------------|
| `long long`          | `long long`        | `long long int`           |
| `long`               | `long`             | `long int`                |
| `short`              | `short`            | `short int`               |
| `unsigned long long` | `unsigned long long` | `long long unsigned int` |
| `unsigned`           | `unsigned int`     | `unsigned int`            |

So any class- or function-template spec with a `long`/`long long`/`short`/unsigned
sibling in its argument list got a backend-dependent Python name. This is a general
classifier divergence, not fmt-specific.

## Fix

Added `builtin_camel_fragment(info type)` in nb_reflect.h: a compiler-neutral table
mapping each canonical builtin scalar type (reflection-equality, not text) to a fixed
CamelCase fragment, consulted before the identifier/display fallback in `spec_camel_name`.
The fragments reproduce EXACTLY what clang's old display path produced
(`sanitize_identifier` + `capitalize_first` of clang's spelling, e.g. `Longlong`,
`Signedchar`, `Char8_t`, `Longdouble`), so NO clang-backend name changes; GCC's
divergent integral spellings are pinned to the same value. `wchar_t` is deliberately
left to the fallback (clang spells it `int`, GCC `wchar_t`; nothing depends on it —
noted in-code to pin it if a run ever needs it). Because `spec_camel_name` is the shared
classifier, both the constexpr and emit backends pick up the fix identically.

## Files touched

- `nanobind/include/nanobind/nb_reflect.h` — new `builtin_camel_fragment`; one extra
  branch in `spec_camel_name`.

## Validation

- fmt run: `outcome=E constexpr=E emit=E surface=pass`.
- Unit suites both backends after the edit: GCC 129 passed, clang 129 passed.

## Note on a unit-suite case

The bug class IS representable in the unit suite (a free/class template spec'd on
`long long`), but no existing fixture spec'd on a wide-integer builtin, so the divergence
was corpus-only. A fixture case (e.g. `identity<long long>`) asserting the CamelCase
name would lock this; left as a follow-up to avoid churning the shared fixture mid-wave.
