dedup_key: reflect-raw-ptr-return-take-ownership-default
layer: BINDER

# nb::reflect_ defaults raw-pointer-returning methods to take_ownership, double-freeing borrowed pointers

## Smallest trigger

A reflected method that returns a *borrowed* raw pointer into a container the C++ object
owns (the classic fluent-builder / accessor pattern). Minimal standalone repro
(`/tmp/ownrepro.cpp`, reproduced here):

```cpp
#include <nanobind/nb_reflect.h>
#include <memory>
#include <vector>
namespace nb = nanobind;
namespace mini {
struct Child { int v = 7; int get() const { return v; } };
class Parent {
    std::vector<std::unique_ptr<Child>> kids_;     // Parent OWNS the children
public:
    Parent() = default;
    Parent(const Parent&) = delete;
    Child* add() { kids_.push_back(std::make_unique<Child>()); return kids_.back().get(); }
    int count() const { return (int)kids_.size(); }
};
}
NB_MODULE(ownrepro, m){ nb::reflect_<^^mini::Parent, ^^mini::Child>(m); }
```

```python
import ownrepro as r
p = r.Parent()
c = p.add()                 # nanobind takes ownership of the borrowed Child*
print(p.count(), c.get())   # prints "1 7"
del c; del p                # -> SIGABRT: double free (Python deletes Child, then ~unique_ptr does too)
```

Result: `EXIT=133` (SIGABRT, libc++ "double free"). Compiles and imports cleanly; the
abort is at the first `del`/GC of the returned wrapper, or at interpreter teardown.

## Diagnosis

For a method returning a bare pointer `T*`, `nb::reflect_` applies the default
`rv_policy::automatic` (see `nb_reflect.h` `ann_rv_policy()` -> `rv_policy::automatic` when no
`[[=reflect::return_policy]]` annotation is present). In nanobind, `automatic` for a pointer
return resolves to **`take_ownership`** -- Python's wrapper assumes it owns the pointee and
`delete`s it on collection. When the C++ owner (here `Parent`, in the field case CLI11's
`App` via `std::vector<std::unique_ptr<Option>>`) also owns it, the result is a double free:
heap corruption, a teardown abort, and -- if the returned wrapper is GC'd while the C++ owner
is still alive and iterates the pointee -- a use-after-free segfault inside the C++ library.

A hand-written `nb::class_<Parent>().def("add", &Parent::add, nb::rv_policy::reference_internal)`
behaves correctly; only the reflection-generated binding crashes. nanobind's own guidance is
that borrowed-pointer returns are extremely common and `take_ownership` is the dangerous
default; `reference` / `reference_internal` (or at least `automatic_reference`) would be the
safe reflection default for a bare-pointer return whose pointee the binder did not allocate.

## Field shape (CLI11 v2.6.2)

CLI11's entire API is fluent/builder-shaped: `App::add_option`/`add_flag` return
`Option*` (owned by `App` through `Option_p = std::unique_ptr<Option>`), and every setter
(`required`, `expected`, `description`, `take_all`, ...) returns `Option*`/`App*` (self).
Through this binder:

- A retained option wrapper -> teardown SIGABRT (double free) -- `EXIT=133`.
- A *discarded* `add_flag(...)` return (very normal in CLI usage) -> the wrapper is GC'd
  immediately, the `Option` is freed, and the next `app.parse(...)` segfaults walking the
  now-dangling pointers. Crash report: `EXC_BAD_ACCESS (SIGSEGV) at 0x8` in
  `CLI::App::_process_requirements()` <- `_process()` <- `_parse(vector<string>&)` <- the
  reflect_ method wrapper for `App::parse`.

So the binder cannot drive CLI11's real `add_*` + `parse` path at all -- the run's
differential (oracle_native.cpp drives the identical specs/argv cleanly) cannot be matched.

## First diagnostics

```
libc++abi: terminating ... double free        (retained-pointer / teardown path, SIGABRT 133)
EXC_BAD_ACCESS (SIGSEGV) KERN_INVALID_ADDRESS at 0x0000000000000008
  frame: CLI::App::_process_requirements()
  frame: CLI::App::_process()
  frame: CLI::App::_parse(std::vector<std::string>&)
  frame: nanobind ... reflect_method_binder<CLI::App, &CLI::App::parse>::...__invoke
  frame: nanobind::detail::nb_func_vectorcall_complex
                                                 (discarded-return / parse path, SIGSEGV 139)
```

## Suggested fix direction (binder)

Default a bare-pointer (`T*`) return to `rv_policy::reference` / `reference_internal`
(self-returning setters especially), not `automatic` (= take_ownership). A returned
smart pointer (`unique_ptr`/`shared_ptr`) still conveys ownership and should keep its
caster semantics; only raw `T*` should change. Annotated `return_policy` continues to win.
