dedup_key: binder-same-name-member-static-method-import-abort

# A class with a non-static method and a static method of the SAME name aborts nanobind at import

Layer: BINDER

## Found via
SQLiteCpp 3.3.3 corpus run (`corpus/runs/sqlitecpp`). `SQLite::Database::getHeaderInfo` exists
as BOTH a const member (`Header getHeaderInfo() const`) and a static overload
(`static Header getHeaderInfo(const std::string&)`). The binder binds the member with `.def()`
and the static with `.def_static()` under the same Python name `getHeaderInfo`; the module then
ABORTS during `import` (SIGABRT) with nanobind's release-mode critical-error message. Gate 4
(compile) passes cleanly -- the failure is at module initialization, Gate 5 (outcome C).

## Smallest trigger
```cpp
#include <nanobind/nb_reflect.h>
namespace n {
struct Conn {
    int getInfo() const { return 1; }          // instance method
    static int getInfo(int x) { return x; }     // static method, SAME name
};
}
namespace nb = nanobind;
NB_MODULE(repro_ext, m) { nb::reflect_<^^n::Conn>(m); }
```
Compiles clean; `import repro_ext` aborts. (Standalone repro committed at
`corpus/runs/sqlitecpp/findings_draft/synth3.cpp`.)

## First diagnostics
```
Critical nanobind error: encountered an unrecoverable error condition.
Recompile using the 'Debug' mode to obtain further information about this problem.
-> SIGABRT during PyInit_<module> (the reflect_ binding of the class)
```
(Release nanobind suppresses the detailed message; the abort is nanobind's `detail::fail`
raised while finalizing the type whose dict has a name bound as both an instancemethod and a
staticmethod.)

## Analysis
The binder's method/static-method binders (`reflect_bind_method` -> `.def`,
`reflect_bind_static_method` -> `.def_static`) each bind under the entity's identifier with no
cross-check that the SAME identifier is not already (or about to be) bound the other way.
nanobind cannot represent one attribute as simultaneously a bound instance method and a
staticmethod, so when the second registration lands it aborts. A C++ class is perfectly
allowed to have a member function and a static member function of the same name (different
parameter lists make them a valid overload set at the C++ level); `SQLite::Database` does
exactly this with `getHeaderInfo`.

Possible fixes (binder side): detect the static/non-static name collision during the member
walk and either (a) merge both into a single nanobind overload chain bound with `.def` that
ignores `self` for the static arm, or (b) gracefully skip the static overload (or the colliding
pair) with a diagnostic, the way other unbindable shapes are skipped -- rather than letting
nanobind abort at import. The current behavior (clean compile, hard abort at import) is the
worst outcome because it is invisible until runtime.

## Run workaround
Excluding the `SQLite::Header` return type via `nb::exclude_<^^SQLite::Header>` makes both
`getHeaderInfo` overloads skip (a method whose signature mentions an excluded type is skipped),
which severs the collision and lets `SQLite::Database` bind head-on. This is also semantically
correct for the in-memory differential (getHeaderInfo parses an on-disk file header), but the
exclusion is forced by the binder bug, not chosen freely.
