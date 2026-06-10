dedup_key: binder-const-void-ptr-return-caster-hard-error

# A method returning `const void*` (pointer-to-void) hard-errors in nanobind's void* caster instead of gracefully skipping

Layer: BINDER

## Found via
SQLiteCpp 3.3.3 corpus run (`corpus/runs/sqlitecpp`). `SQLite::Column::getBlob() const noexcept`
returns `const void*` (and `Statement::bind(int, const void*, int)` / `Column::operator const void*()`
take/return the same). Binding `SQLite::Column` head-on stops Gate 4 with a single hard error;
there is no public handle (annotation/exclude) to skip just these members, so the whole run is
blocked at outcome B.

## Smallest trigger
```cpp
#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
struct Cell {
    int v = 0;
    const void* getBlob() const noexcept { return &v; }
};
NB_MODULE(voidrepro_ext, m) { nb::reflect_<^^Cell>(m); }
```
(Standalone repro committed at `corpus/runs/sqlitecpp/findings_draft/repro_const_void_ptr.cpp`;
build with `corpus/lib/build_module.sh`.)

## First diagnostics
```
nanobind/include/nanobind/nb_func.h:292:24: error: cannot initialize a parameter of type
    'void *' with an rvalue of type 'const void *'
  292 |   cap->func((in.template get<Is>()).operator cast_t<Args>()...)
... instantiation of nanobind::class_<Cell>::def<...>  (nb_reflect.h:467, the const-noexcept method binder)
```

## Analysis
`is_unbindable_shape` (nb_reflect.h ~line 612) is the graceful-skip gate that already drops
ptr-to-ptr, ptr-to-function, and `T*&` out-params. A pointer whose pointee is `void` is NOT
covered: line 624-628 only rejects a pointee that is itself a pointer or a function, so
`const void*` (and `void*`) falls through to "bindable". The method binder then synthesizes a
lambda returning `const void*` and hands it to nanobind's `void*` type-caster, whose
`cast_t<const void*>` strips to a non-const `void*` parameter -- a const-mismatch HARD error
(TU-wide), exactly the failure mode `is_unbindable_shape` was introduced to prevent for the
other unrepresentable pointer shapes.

`void` / `const void*` has no Python representation in this binder (nanobind's `void*` caster
exists but does not round-trip `const void*`, and an opaque void blob is not meaningful to
Python anyway). The natural fix is to treat pointer-to-(possibly-cv)-void as another
unbindable shape in `is_unbindable_shape` (return type AND parameter), so blob accessors are
gracefully skipped on every path (binding + the STL caster-collection walk) -- the same
treatment ptr-to-ptr already gets. That would unblock binding `Column`/`Statement` head-on
(getInt/getDouble/getText/getString cover the typed columns; the blob path is legitimately
out of Python's reach).

## Adjacent toolchain observation (separate dedup_key, lower confidence)
While minimizing, an OVER-minimal repro (a `reflect_<^^Cell>` whose TU includes ONLY
`nb_reflect.h`, no `<nanobind/stl/*.h>`) ICEs the toolchain instead -- the binder's
`bind_free_operators` -> `is_bindable_free_operator` scan of the enclosing (global) namespace
calls `return_type_of` on a namespace-scope function-template reflection, whose error/diagnostic
path reaches `clang::DescriptionOf` and hits
`llvm_unreachable("unhandled template kind")` at `clang/lib/AST/ExprConstantMeta.cpp:1772`
(the `ReflectionKind::Template` switch lacks a default). This does NOT fire in the real
sqlitecpp binding (which includes `<nanobind/stl/string.h>` and got past the operator scan to
the `const void*` caster error), so it is an environment-dependent artifact of the free-operator
namespace walk, not the run's blocker -- but `DescriptionOf` having an unreachable default for a
template kind that `return_type_of` can be asked to describe is a constant-evaluator robustness
gap worth hardening (category E). Smallest trigger:
```cpp
#include <nanobind/nb_reflect.h>   // and NOTHING else from nanobind/stl
namespace nb = nanobind;
struct Cell { int x = 0; };
NB_MODULE(ice_ext, m) { nb::reflect_<^^Cell>(m); }
```
ICE backtrace top: `DescriptionOf` <- `return_type_of` <- `is_bindable_free_operator`.
dedup_key for this one: `toolchain-descriptionof-template-kind-unreachable`.
