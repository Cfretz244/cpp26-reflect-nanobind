# BINDER-0012 — deleted constructors are bound; any class with one is a TU-wide hard error

- **Status:** DRAFT / open (found by the corpus, not yet fixed — per the run-agent
  guardrails the binder was not touched)
- **Found via:** `corpus/runs/expected` (Tier 3, TartanLlama/expected v1.3.1).
  `tl::unexpected<E>` declares `unexpected() = delete;`; binding any
  `tl::unexpected<E>` spec — or any `tl::expected<T,E>` spec, which makes
  `unexpected<E>` signature-reachable (below) — fails Gate 4 with
  `nb_class.h:366: error: call to deleted constructor of 'Alias'
  (aka 'tl::unexpected<std::string>')`.
- **Files:** `nanobind/include/nanobind/nb_reflect.h` (`bind_class_contents`,
  constructor pass).

## Root cause

The constructor pass binds every enumerated constructor that is public,
non-template, and not the copy/move constructor:

```cpp
if constexpr (!is_using_proxy(fn)
    && std::meta::is_constructor(fn)
    && std::meta::is_public(fn)
    && !std::meta::is_template(fn)
    && !std::meta::is_copy_constructor(fn)
    && !std::meta::is_move_constructor(fn)) {
    reflect_bind_ctor<fn>(cls);
}
```

There is no deleted-ness filter. A `= delete`d constructor is a public,
enumerable member declaration, so `members_of` yields it and the pass emits
`cls.def(nb::init<...>())` for it; `nb::init`'s dispatch lambda then calls the
deleted constructor — an immediate hard error that kills the whole TU (same
blast-radius shape as BINDER-0011's abstract-class `nb::init`).

`= delete` on a constructor is one of the most common C++ idioms there is
(every "no default construction" vocabulary type), so this bites broadly. It
also cannot be dodged by leaving the offending class out of the `reflect_`
pack when the class is signature-reachable: `tl::expected<T,E>`'s
`operator=(unexpected<G>&&)` member template is default-instantiable
(`G = E`), so the user-spec discovery walk re-adds `unexpected<E>` to the bind
set behind the user's back.

## Minimal repro (no library needed)

```cpp
#include <nanobind/nb_reflect.h>
namespace demo {
struct NoDefault {
    NoDefault() = delete;
    explicit NoDefault(int v_) : v(v_) {}
    int v;
};
}
NB_MODULE(x, m) { nanobind::reflect_<^^demo::NoDefault>(m); }
// nb_class.h:366: error: call to deleted constructor of 'Alias' (aka 'demo::NoDefault')
```

## Suggested fix (not applied)

Add `!std::meta::is_deleted(fn)` to the constructor pass (and audit the other
function-binding passes — a deleted member function or deleted free operator
has the same shape; methods/operators are rarer but `operator=(const T&) =
delete` style members exist). After the filter, a class whose ONLY
constructors are deleted naturally binds with no `__init__`, which is exactly
the Python-side TypeError contract BINDER-0011 established for abstract
classes — `tl::unexpected<E>` would then bind its real
`unexpected(const E&)`/`unexpected(E&&)` ctors and Python `unexpectedString()`
raises TypeError.

## Field impact in the corpus

`corpus/runs/expected` records outcome **B**; at codegen (`-c`, what Gate 4
runs) this error is preempted by the TC-0008 mangler ICE (tl's namespace-scope
deduction guide in the binder's free-operator walk), so this finding is the
diagnostic `-fsyntax-only` shows and the NEXT blocker once TC-0008 lands. The
run's tests/oracle are written for the intended surface so it re-runs green
once both land.
