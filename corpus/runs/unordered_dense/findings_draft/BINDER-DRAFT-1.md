# BINDER-DRAFT-1 — libstdc++ implementation-namespace types leak into user-spec discovery

dedup_key: `binder-is-in-std-reserved-impl-namespaces`

## Symptom

gcc16 backend, unordered_dense run, constexpr lane: Gate 4 compile error deep in
`nb_func.h`, instantiating `reflect_init<std::pair<int,std::string>* const&>` for
`nanobind::class_<__gnu_cxx::__normal_iterator<std::pair<int,std::string>*,
std::vector<...>>>` — i.e. the binder was trying to BIND std::vector's iterator
class (including its pointer constructor) as if it were a user type. Never seen
on the clang lane.

## Root cause

`is_in_std` (nb_reflect.h) classified an entity as standard-library by checking
for an enclosing namespace literally named `std`. That matches libc++'s layout
(everything, including `std::__wrap_iter`, lives under `std::__1`), but
libstdc++ keeps vendor internals OUTSIDE std: `std::vector<T>::iterator`
dealiases to `__gnu_cxx::__normal_iterator<T*, vector<T>>`. The user-spec
discovery fixpoint (`is_user_class_template_spec` /
`collect_user_specs_from_type`) therefore saw the iterator as a *user*
class-template specialization the moment any bound signature mentioned it
(unordered_dense's `table::begin/end/find/erase(iterator)`), pulled it into the
bind set, and binding its ctor surface fails to compile (and would be a surface
divergence vs the clang record even if it compiled).

## Fix

`is_in_std` now also treats any enclosing namespace whose name is reserved to
the implementation (leading `__`, or `_` + capital: `__gnu_cxx`, `__gnu_debug`,
`__cxxabiv1`, ...) as standard-library territory. Compiler-neutral (no
`#ifdef`): on libc++ such namespaces only occur under std anyway, and user code
cannot legally claim reserved names. All `is_in_std` consumers want exactly
this semantic ("belongs to the stdlib implementation"): spec_camel_name's
friendly names, `stl_caster_header` (name match still fails for internal
types → no caster demand, unchanged), the completeness gates, and the
user-spec discovery walk that motivated the fix.

## Files touched

- `nanobind/include/nanobind/nb_reflect.h` (`is_in_std`)
- `nanobind/tests/test_reflect_fixture.h` + `nanobind/tests/test_reflect.cpp`
  (regression: signature mentioning `std::vector<int>::iterator`, static_assert
  the dealiased iterator class is never discovered as a user spec — on either
  stdlib)
