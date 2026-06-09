# TC-0001 — Missing Apple type-aware allocation operators in from-source libc++abi (FIXED)

- **Status:** root-caused + **fixed in the pinned toolchain**
- **Kind:** runtime/packaging gap — clang codegen ↔ from-source libc++abi ABI mismatch
- **Signature:** `dyld: Symbol not found: __ZnamSt19__type_descriptor_t`
- **Toolchain:** `llvm-project` clang-p2996; `libcxxabi/src/CMakeLists.txt`, `libcxxabi/lib/new-delete.exp`
- **Discovered:** Phase 0a (native oracle); blocked nlohmann/json's L1 differential.

## Root cause (was mislabeled "-O0 only / typed new")

Recent clang targeting macOS emits calls to Apple's **type-aware allocation** operators
`operator new[]/new/delete(..., std::__type_descriptor_t)`. Those symbols exist in Apple's
**system** libc++ and in the vendor shim `libcxxabi/src/vendor/apple/shims.cpp`, but that shim is
**not in `LIBCXXABI_SOURCES`** — the normal libc++abi build never compiles it. So a from-source
libc++abi is missing the symbols; clang's codegen references them; the program aborts at load.
`-O2`/`-O3` only *sometimes* optimize the reference away (linalg/glm dodged it; **json hits it at
every -O level** because its aggregate-heavy code keeps the call live).

## Fix

- `libcxxabi/src/CMakeLists.txt`: compile `vendor/apple/shims.cpp` on `APPLE` (gated with the
  new/delete definitions).
- `libcxxabi/lib/new-delete.exp`: export the 10 typed operators (the Apple `-exported_symbols_list`
  otherwise hides them as local symbols even once compiled).

The shim forwards each typed operator to the untyped one — the descriptor is an optional
type-aware-allocation hint (Apple's own shim does the same; "using" it would require Apple's
private `malloc_type_*` API and yields only allocator hardening, not correctness).

Rebuild: `ninja -C toolchain-build/runtimes/runtimes-bins cxxabi_shared && ... install-cxxabi`
(`touch llvm-project/libcxxabi/src/vendor/apple/shims.cpp` first — the `.exp` isn't a tracked link
input). After the fix, `nm -gU toolchain/lib/libc++abi.1.0.dylib | grep type_descriptor` shows the
operators as exported (`T`), the native oracle runs, and json reaches **outcome E**.

## Historical note (superseded below)

The original finding (kept for the record) framed this as "-O0-only typed `operator new[]`,
dodged by -O2." That workaround (`build_native.sh -O2`) is now unnecessary; the real fix is in the
toolchain.

## Symptom

Any executable that uses an aggregate **with a member function** and is compiled at **-O0**
references `operator new[](unsigned long, std::__type_descriptor_t)` as an *undefined weak* symbol.
No library in `toolchain/lib` (`libc++`, `libc++abi`, `libc++experimental`, `libunwind`) defines it,
so the program aborts at load:

```
dyld: Symbol not found: __ZnamSt19__type_descriptor_t
      Expected as weak-def export from some loaded dylib
```

## Minimal reproducer

```cpp
struct P { double x, y; double n() const { return x*x + y*y; } };
int main() { P p{3,4}; return (int)p.n(); }
```
```
clang++ -std=c++26 -stdlib=libc++ -isysroot "$(xcrun --show-sdk-path)" \
        -nostdinc++ -isystem toolchain/include/c++/v1 -arch arm64 \
        -L toolchain/lib -Wl,-rpath,toolchain/lib repro.cpp -o repro
DYLD_LIBRARY_PATH=toolchain/lib ./repro   # -> dyld Symbol not found, abort

clang++ ... -O2 repro.cpp -o repro        # -O2: the spurious reference is gone; runs clean
```

Notes from bisection:
- No `-freflection-latest` needed — plain `-std=c++26` triggers it.
- A struct/aggregate *without* a member function, or a program using only scalars, does NOT trigger it.
- The reference is **undefined weak** (hidden from `nm -u`; visible via runtime dyld failure).
- This looks like the P2719-style *type-aware allocation* hook being emitted but left unbound.

## Why it didn't block the binder before

The nanobind binding modules build at **-O3 -Os**, which elides the reference — so the existing
31 reflection tests never hit it. It only surfaced when building a plain, unoptimized oracle.

## Workaround in the corpus

`corpus/lib/build_native.sh` compiles oracle harnesses at **-O2**. Documented inline there.

## Upstream

TODO: minimize further (does a libc++ header pull in a typed `operator new[]` declaration that the
optimizer normally drops?) and file against the clang-p2996 fork. dedup_key: `typed-new-array-O0`.
