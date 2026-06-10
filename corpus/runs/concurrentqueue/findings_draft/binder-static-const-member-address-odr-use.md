dedup_key: binder-static-const-member-bound-by-address-odr-use
layer: BINDER

# Binder binds in-class-initialized `static const` data members by address, ODR-using them -> undefined symbol at link

## Summary

`reflect_bind_static_member` (nb_reflect.h) binds a class's static data members by taking
their address (it forms a pointer-to-static `&Class::MEMBER` -- the mangled reference is
`nanobind::detail::reflect_bind_static_member<...>`). For a `static const`/`static constexpr`
integral data member that was given an **in-class initializer but never defined out-of-line**,
taking its address ODR-uses the member, which (pre-C++17 rules, or for any never-`inline`d
const static) requires a namespace-scope definition the library does not provide. The result
is an **undefined symbol at link** -- the binding compiles but fails to link.

This is the textbook "in-class static const has no storage unless odr-used + defined out of
line" situation. The binder should bind such constants **by value** (their value is available
to reflection / is a constant expression) rather than by address, or skip static data members
that have no definition. Binding by value also matches how Python users would want a class
constant exposed (an attribute holding the int, not a property over a pointer).

## Smallest trigger (standalone, no library needed)

```cpp
#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
namespace mini {
struct S {
  static const int K = 7;        // in-class init, NO out-of-line definition
  int get() const { return 1; }
};
}
NB_MODULE(mini_ext, m) { nb::reflect_<^^mini::S>(m); }
```

Build with the corpus `build_module.sh`. Compiles clean, then:

```
Undefined symbols for architecture arm64:
  "mini::S::K", referenced from:
      nanobind::detail::reflect_bind_static_member<mini::S, &mini::S::K, ...> in mini_ext.o
ld: symbol(s) not found for architecture arm64
```

## Where it bit in the corpus

moodycamel::ConcurrentQueue<T> (concurrentqueue v1.0.5) declares seven such constants:
`BLOCK_SIZE`, `MAX_SUBQUEUE_SIZE`, `EXPLICIT_INITIAL_INDEX_SIZE`,
`IMPLICIT_INITIAL_INDEX_SIZE`, `INITIAL_IMPLICIT_PRODUCER_HASH_SIZE`,
`EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD`, `EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE`
(all `static const size_t` / `static const std::uint32_t` with in-class initializers, none
defined out-of-line). Binding ConcurrentQueue<int> and ConcurrentQueue<std::string>
head-on produced 14 undefined symbols (7 per spec) -- see first diagnostics below.

## First diagnostics (concurrentqueue, before workaround)

```
Undefined symbols for architecture arm64:
  "moodycamel::ConcurrentQueue<int, moodycamel::ConcurrentQueueDefaultTraits>::BLOCK_SIZE",
   referenced from:
      nanobind::detail::reflect_bind_static_member<...ConcurrentQueue<int...>...,
        &...::BLOCK_SIZE, nanobind::class_<...>> in concurrentqueue_ext.o
  ... (MAX_SUBQUEUE_SIZE, EXPLICIT_INITIAL_INDEX_SIZE, IMPLICIT_INITIAL_INDEX_SIZE,
       INITIAL_IMPLICIT_PRODUCER_HASH_SIZE, EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD,
       EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE, x2 specs) ...
ld: symbol(s) not found for architecture arm64
```

## Workaround used in this run (so the run could proceed)

Per AGENT_PROMPT (drop the triggering entity, record in skipped_features), the run excludes
all seven static-data-member reflections on both specs via a programmatic `nb::exclude_<...>`
marker built in binding/binding.cpp (`cq_excluded_marker()`), collecting members where
`is_static_member(m) && is_variable(m)` and wrapping each in `reflect_constant`. With those
excluded the run links and reaches outcome E. The constants are configuration knobs, not part
of the producer/consumer differential, so excluding them costs the run nothing.

## Suggested fix direction (binder, not applied here)

In `reflect_bind_static_member`, for a static data member that is `const`/`constexpr` of
literal type, read its compile-time value via reflection and bind it as a plain attribute
(by value) instead of `def_*`-ing over `&Class::MEMBER`. That avoids the ODR-use entirely and
is also the more Python-natural representation of a class constant. (A narrower fix -- skip
static data members lacking an out-of-line definition -- would also unblock, but loses the
constant; binding by value is strictly better.)
