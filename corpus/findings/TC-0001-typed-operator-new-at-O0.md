# TC-0001 — Undefined weak `operator new[](size_t, std::__type_descriptor_t)` at -O0

- **Status:** suspected (toolchain)
- **Kind:** wrong-codegen / unresolved-weak-symbol
- **Signature:** `__ZnamSt19__type_descriptor_t` referenced as undefined weak; no toolchain lib defines it
- **Toolchain:** `llvm-project` submodule @ `d4ae403` (clang-p2996), its bundled libc++
- **Discovered:** Phase 0a, building the native differential oracle.

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
