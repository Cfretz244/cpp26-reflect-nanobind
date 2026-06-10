dedup_key: binder-init-braced-init-hijacked-by-initializer-list-ctor

# Reflected ctor binding uses braced-init `Type{args...}`, hijacked by an `initializer_list` ctor

layer: BINDER

## Smallest trigger

A class that has BOTH

1. an `std::initializer_list<T>` constructor, and
2. another constructor whose argument list, when written as a braced-init-list,
   would prefer that `initializer_list<T>` constructor with a narrowing conversion.

`immer::vector<int>` is exactly this shape:

```cpp
vector(std::initializer_list<T> values);   // (1)
vector(size_type n, T v = {});             // (2)  size_type == std::size_t
```

The binder reflects ctor (2) and emits `nb::init<unsigned long, int>`. nanobind's
`init<Args...>::execute` constructs the instance with **braced** init:

```cpp
// nb_class.h:362 / :366
new (v.p) Type{ (detail::forward_t<Args>) args... };   // Type{ unsigned long, int }
```

`Type{ unsigned long, int }` does NOT select `vector(size_type, T)` — braced-init-lists
prefer an `initializer_list<int>` constructor, and `unsigned long -> int` inside that list
is a narrowing conversion, so compilation fails:

```
nb_class.h:366:43: error: non-constant-expression cannot be narrowed from type
'unsigned long' to 'int' in initializer list [-Wc++11-narrowing]
note: in instantiation of 'nanobind::init<unsigned long, int>::execute<
      nanobind::class_<immer::vector<int>>, ...>' requested here
... reflect_bind_ctor<^^(declaration), class_<immer::vector<int>>> ...
```

## Root cause

`init<>::execute` uses braced initialization `Type{args...}` rather than parenthesized
`Type(args...)`. For any reflected class that also exposes an `initializer_list<T>` ctor,
braces hijack overload resolution toward the `initializer_list` ctor; when the reflected
ctor's args narrow to fit that list it is a hard error, and even when they don't narrow it
silently calls the WRONG constructor (a single-element `{n}` list rather than the
`(n, value)` fill ctor). The reflected constructor `vector(size_type, T)` can never be
invoked through this path.

This is independent of immer: any library binding a container-like class with an
`initializer_list` ctor plus a sized/fill ctor hits it. Parenthesized init
`Type(args...)` (where the class is constructible from `Args...`) would select the intended
ctor; `init<>` already gates on `std::is_constructible_v<Type, Args...>`.

## Workaround in this run

The 2-arg fill constructor `vector(size_type, T)` is excluded per-member via `nb::exclude_`
(enumerated from `members_of`), so the rest of the persistent-container surface binds. The
default ctor and the persistent push_back/set chain are unaffected. Recorded in
skipped_features.

## First diagnostics

```
nanobind/include/nanobind/nb_class.h:366:43: error: non-constant-expression cannot be
narrowed from type 'unsigned long' to 'int' in initializer list [-Wc++11-narrowing]
```
