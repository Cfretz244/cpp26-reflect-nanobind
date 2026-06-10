dedup_key: exclude-and-completeness-not-recursing-into-stdfunction-arg-types
layer: BINDER

# nb::exclude_ (and the completeness gate) do not look inside std::function argument types

## Summary

When a member's parameter is a `std::function<R(..., T, ...)>` whose *argument* type `T`
is an excluded (or otherwise un-representable) class, the binder still binds that overload,
exposing `T` raw in the registered Python signature. `exclude_<T>` makes `T` opaque on the
DIRECT parameter/return path (overloads taking a bare `T&`/`T` are correctly skipped), but
the exclusion is NOT propagated through the argument-type list of a `std::function`
parameter. The result is a bound-but-uncallable overload whose Python signature names an
unbound C++ type, e.g.:

    Post(self, path: str, content_length: int,
         content_provider: collections.abc.Callable[[int, int, httplib::DataSink], bool],
         content_type: str, progress: collections.abc.Callable[[int, int], bool]) -> Result

Here `httplib::DataSink` is in the run's `nb::exclude_<...>` pack and is correctly NOT
bound as a class (`hasattr(m, "DataSink") is False`), yet it surfaces verbatim inside the
`Callable[...]` of this overload. The overload can never be satisfied from Python (no
`DataSink` value is constructible), so it is dead surface, not a crash -- but it contradicts
exclude_'s stated contract ("make a type opaque on EVERY path so methods mentioning it are
skipped", BINDER-0014) and pollutes the public signature.

Mirror of the same gap on the completeness side: if `<nanobind/stl/function.h>` is included,
the function caster is considered present, so the BINDER-0014 completeness gate is satisfied
for the `std::function` parameter as a whole and never inspects whether the inner argument
types have casters / are excluded. So a `std::function` argument type that has no caster
slips through the gate the same way an excluded one slips through exclude_.

## Smallest trigger

A class with a method taking `std::function<bool(Opaque&)>`, where `Opaque` is passed in
`nb::exclude_<^^Opaque>` (and `<nanobind/stl/function.h>` is included):

    struct Opaque { int x; };
    struct C { void f(std::function<bool(Opaque&)> cb); };
    // nb::reflect_<^^C, ^^nb::exclude_<^^Opaque>>(m);
    // -> C.f binds with signature f(self, cb: Callable[[Opaque], bool]),
    //    even though Opaque is excluded and unbound.

In this run: bind `httplib::Client` with `^^httplib::DataSink` in the exclude_ pack and
`<nanobind/stl/function.h>` included; inspect `m.Client.Post.__doc__` -- the
`ContentProvider` overloads (`content_provider: Callable[[..., httplib::DataSink], bool]`)
bind and name the excluded `DataSink`.

## Expected vs actual

- Expected: an overload whose `std::function` parameter mentions an excluded / caster-less
  type in its argument list should be SKIPPED, exactly as if the type appeared as a direct
  parameter (exclude_'s "every path" contract). At minimum the completeness gate should
  reject a `std::function` whose inner argument types lack casters, rather than passing once
  `function.h` is present.
- Actual: the overload binds; the excluded/un-representable type appears raw in the Python
  signature; the overload is permanently uncallable.

## Impact / workaround

Low severity in this run: the leaked overloads are uncallable but the bindable overloads
(`Post(path, body, content_type, progress)` etc.) work, and the dead signatures do not break
overload resolution for the calls the tests actually make. No workaround was applied (the run
lands E with the dead surface present). A complete fix would recurse the exclude_/completeness
check into `std::function`/callable argument (and return) types.

## First diagnostics

No diagnostic -- silent. Observed only by inspecting the bound `__doc__` overload list. The
overload binds and imports clean; calling it from Python fails with the generic
"incompatible function arguments" TypeError because no `httplib::DataSink` argument can be
produced.
