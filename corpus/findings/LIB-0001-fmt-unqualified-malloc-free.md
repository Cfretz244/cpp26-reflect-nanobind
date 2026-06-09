# LIB-0001 — fmt calls unqualified `malloc`/`free` without `<cstdlib>` (library latent bug)

- **Status:** root-caused + **worked around consumer-side** (no fmt edit needed)
- **Kind:** library latent bug exposed by strict two-phase name lookup (triage: `library`, not
  `toolchain` or `binder`)
- **Signature:** `error: call to function 'free' that is neither visible in the template
  definition nor found by argument-dependent lookup`
- **Library:** fmtlib/fmt 11.2.0 (`include/fmt/format.h`, `detail::allocator<T>`)
- **Discovered:** Gate 1 probe of `fmt` (Tier 2).

## Root cause

`<fmt/format.h>` defines `detail::allocator<T>` whose members call the **unqualified, global**
`malloc()`/`free()`:

```cpp
// include/fmt/format.h
template <typename T> struct allocator {
  T* allocate(size_t n) { ... T* p = static_cast<T*>(malloc(n * sizeof(T))); ... }
  void deallocate(T* p, size_t) { free(p); }   // <-- line 752
};
```

fmt never `#include <cstdlib>` (it lives only in `fmt/std.h`) and never qualifies `std::malloc`/
`std::free`. `free` here is a **non-dependent** unqualified name, so C++ two-phase lookup resolves
it at the **template-definition** point — and if no `free` has been declared yet, the program is
ill-formed (no ADL can rescue it: the argument `T*` has no associated namespace that contains
`free`). fmt has worked on most setups only because some other header transitively declared
`::free`/`::malloc` (or the global) before `format.h` was parsed. clang-p2996 (a recent clang) is
strict about this and rejects it.

This is **not C++26-specific**: the standalone reproducer below is rejected at `-std=c++17` too.
It is a genuine fmt portability bug; the correct upstream fix is for fmt to `#include <cstdlib>`
and call `std::malloc`/`std::free`.

## Minimal reproducer (no fmt, no reflection)

```cpp
// No <cstdlib>. A class-template method calls unqualified global free().
template <class T> struct allocator { void deallocate(T* p) { free(p); } };
int main() { allocator<int> a; a.deallocate(nullptr); return 0; }
```
```
clang++ -std=c++26 -stdlib=libc++ -isysroot "$(xcrun --show-sdk-path)" \
        -nostdinc++ -isystem toolchain/include/c++/v1 -fsyntax-only repro.cpp
# error: use of undeclared identifier 'free'   (same at -std=c++17)
```

## Workaround (in this corpus)

Consumer-side, **no library edit**: make `<cstdlib>` visible before `<fmt/format.h>` is parsed, so
`::malloc`/`::free` are declared at the template-definition point. The `fmt` run's probe, binding,
and oracle TUs all `#include <cstdlib>` first (`runs/fmt/binding/fmttest.h`,
`runs/fmt/probe/probe.cpp`). This is the rule-of-thumb "patch/work around a legitimate library
bug" path — a productionized binding of fmt would do the same (or carry the upstream one-line fix).

Because the workaround is a single consumer-side include and any real fmt TU effectively does it,
this is **not** treated as outcome A (intractable under the toolchain); `fmt` reaches **outcome E**.

## Upstream

Candidate upstream fix for fmtlib/fmt: in `include/fmt/format.h`, add `#include <cstdlib>` and use
`std::malloc`/`std::free` in `detail::allocator<T>`. dedup_key: `fmt-unqualified-malloc-free`.
