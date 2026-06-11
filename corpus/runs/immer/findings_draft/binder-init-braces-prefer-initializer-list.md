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

## Resolution (BINDER-0026, landed)

Fixed in the binder: reflected constructors now construct with PARENS via `reflect_init`
(`nanobind/include/nanobind/nb_paren_init.h`), whose `reflect_construct_at<T>` does
`new (p) T(args...)` (parens) when `is_constructible_v<T, Args...>`, falling back to braces
only for the aggregate case. Parens select exactly the reflected overload, so the fill ctor
`vector(size_type, T)` binds correctly -- `Type(n, v)` is the fill ctor, never the
`initializer_list` ctor. Both backends emit it: the constexpr lane through `reflect_init`
directly, the emit lane renders `cls.def(::nanobind::detail::reflect_init<unsigned long,
int>(), ...)` into the generated source (built by Apple Clang with no toolchain).

The per-member `nb::exclude_` workaround for the fill ctor was REMOVED in the emit-lane wave;
the fill ctor is now bound and Python-callable, and `IVec(3, 7)` -> `[7, 7, 7]` is asserted on
both backends by `tests/test_bindings.py::test_vector_fill_ctor_uses_parens_not_initializer_list`.
(Only the `immer::detail` namespace exclusion remains.)

## First diagnostics

```
nanobind/include/nanobind/nb_class.h:366:43: error: non-constant-expression cannot be
narrowed from type 'unsigned long' to 'int' in initializer list [-Wc++11-narrowing]
```
