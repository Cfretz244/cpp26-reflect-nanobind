# BINDER-0005 — nanobind `concat()` signature builder is ADL-hijackable (FIXED)

- **Status:** fixed in nanobind (`mk-reflect`)
- **Kind:** robustness bug in nanobind core (argument-dependent lookup hijack)
- **Found via:** Phase 1, nlohmann/json — binding `nlohmann::detail::json_default_base` (a base of
  `basic_json`) and `std::function<bool(int, detail::parse_event_t, basic_json&)>`.

## Symptom

Binding any C++ type that lives in a namespace which **also defines a function named `concat`**
fails to compile deep inside nanobind's signature machinery:

```
nb_func.h: error: invalid operands to binary expression ('descr<...>' and 'std::string')
... in instantiation of 'nlohmann::detail::concat<std::string, descr<...>>' requested here
```

## Root cause

nanobind builds each bound function's compile-time signature with
`detail::concat(type_descr(make_caster<Args>::Name)...)` (`nb_func.h`, `nb_descr.h`, the STL
casters). The call was **unqualified**, and a `descr<N, Ts...>` carries the bound C++ types
`Ts...` as template arguments — so the `descr` values' **associated namespaces** include those
types' namespaces. When a bound type is in `nlohmann::detail` (which has its own
`nlohmann::detail::concat`), ADL pulls that in; json's `concat` is an unconstrained variadic that
returns `std::string`, so it shadows/derails nanobind's `descr`-based `concat`.

A first attempt to suppress ADL via the `(concat)(...)` parenthesization idiom broke nanobind's
*own* `concat`, whose recursion in a trailing-return-type relied on ADL-at-instantiation to find
its later-declared overloads.

## Fix

- Rewrote the multi-argument `concat` (`nb_descr.h`) as a **fold over `operator+`** — no recursive
  `concat` call at all (`descr`'s `operator+` is not shadowed by any json overload), so ADL is moot
  and the recursion hazard is gone.
- Suppressed ADL at the entry call sites with `(concat)(...)` / `(concat_maybe)(...)`:
  `nb_func.h`, `nb_cast.h` (typed/callable casters), `stl/{tuple,pair,function}.h`, `ndarray.h`,
  `nb_descr.h` (union_name).

## Validation

Regression-clean: nanobind reflection suite 41/41, plus `test_stl.py` + `test_functions.py`
(141 passed) — these directly assert signature strings and exercise pair/tuple/function casters.
Generally useful beyond json: any library with a `detail::concat` (or similarly-named helper)
would have tripped this.
