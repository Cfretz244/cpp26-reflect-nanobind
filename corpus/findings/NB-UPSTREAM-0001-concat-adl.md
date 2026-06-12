# NB-UPSTREAM-0001 — nanobind `detail::concat` ADL hijack (upstream contribution, FILED)

The one nanobind-core patch mirrorbind's default fork carries (see
mirrorbind/PATCHES.md), prepared as a general-purpose upstream contribution
to wjakob/nanobind. **Framing rule: no reflection/binder/project references
anywhere in the branch or PR** — it is a standalone bug fix whose real-world
trigger is binding types from namespaces that declare their own `concat`
(canonically `nlohmann::detail`).

## State: FILED

- **PR: [wjakob/nanobind#1372](https://github.com/wjakob/nanobind/pull/1372)**
  (filed 2026-06-12; once merged, mirrorbind's default nanobind dependency can
  move from the Cfretz244 fork back to upstream, and PATCHES.md updates).
- Branch: `fix-concat-adl` on `Cfretz244/nanobind`
- Commit: `6fe25595c320c044c953af98e336a7f9dcc1deb0` (one commit on upstream
  master `367ba7ab`)
- Validation: full upstream suite on macOS arm64 / Apple Clang 17 /
  Python 3.12 — 388 passed, 0 failed (168 optional-dep skips); the new
  regression test (`test_58_concat_adl`, tests/test_functions.cpp) verified
  to FAIL to compile against pristine upstream headers
  (`nb_func.h:178: invalid operands ... 'descr<2UL - 1>' and 'std::string'`).

## PR title

```
Avoid ADL when invoking the internal 'concat' helper
```

## PR body

```markdown
### Problem

`nanobind::detail::descr<N, Ts...>` carries the bound C++ types `Ts...` as
template arguments, so the associated namespaces of a `descr` value (for
argument-dependent lookup) include the namespaces of those types. The variadic
`detail::concat` was written as a self-recursive *unqualified* call:

​```cpp
template <size_t N, typename... Ts, typename... Args>
constexpr auto concat(const descr<N, Ts...> &d, const Args &...args)
    -> decltype(std::declval<descr<N + 2, Ts...>>() + concat(args...)) {
    return d + const_name(", ") + concat(args...);
}
​```

The recursive call is resolved at instantiation time using ADL. If a bound type
lives in a namespace that *also* declares a function named `concat`, that
declaration enters the candidate set. Worse, the arguments at nanobind's
internal call sites are typically prvalues (e.g. the results of
`type_descr(...)`), so a foreign forwarding-reference overload such as

​```cpp
template <typename OutStringType = std::string, typename... Args>
OutStringType concat(Args &&...args);
​```

binds rvalue references directly and is a *strictly better match* than
nanobind's `const descr<...> &` overloads — partial ordering never gets a
chance to prefer the more specialized overload. The result is a hard compile
error deep inside nanobind's headers, far from the user code that triggered it:

​```
nb_func.h:178: error: invalid operands to binary expression
('descr<2UL - 1>' and 'std::string')
​```

This is not hypothetical: the declaration above is the exact shape of
`nlohmann::detail::concat` in nlohmann/json. Users binding types declared in
`nlohmann::detail` (iterators, SAX interfaces, the `value_t` enum, exception
types, ...) — or in any other namespace that declares a `concat` — hit this.

### Fix

Two parts, no behavior change (generated signatures are identical):

1. **`nb_descr.h`**: rewrite the variadic `concat` as a fold over
   `descr::operator+`, which lives in `nanobind::detail` and cannot be
   hijacked. This removes the recursive unqualified call (and with it the ADL
   exposure) entirely:

   ​```cpp
   return (d + ... + (const_name(", ") + args));
   ​```

2. **Call-site hardening**: parenthesize the remaining internal unqualified
   calls — `(concat)(...)` / `(concat_maybe)(...)` — in `nb_cast.h`,
   `nb_func.h`, `ndarray.h`, `stl/function.h`, `stl/pair.h`, and
   `stl/tuple.h`. A parenthesized callee suppresses ADL, so only nanobind's
   own overloads are ever considered.

### Test

`tests/test_functions.cpp` now declares a namespace containing a small struct
and a `concat` function template mirroring the nlohmann/json declaration, then
binds the struct plus a two-parameter function returning `std::pair`, which
instantiates the variadic concatenation paths in both `nb_func.h` and
`stl/pair.h`. Without the fix, the test extension fails to compile with the
error shown above. The Python side checks the bound function round-trips
values and renders the expected docstring/signature.

Verified on macOS arm64 (Apple Clang 17, Python 3.12): the full test suite
passes, and reverting the header changes while keeping the test reproduces the
compile failure.
```

(Strip the zero-width markers from the nested code fences when copying.)
