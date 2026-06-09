# CLAUDE.md — cpp26-reflect-nanobind (umbrella prove-out)

Guidance for Claude Code (claude.ai/code). This repo is the **umbrella** that captures an
ongoing, **this-laptop** investigation: using **C++26 static reflection (WG21 P2996)** to
**automatically generate Python bindings** with **nanobind**, plus standalone
reflection-driven demos. It is a prove-out, not a product. Paths below are deliberately
specific to this machine (a macOS / Apple Silicon laptop).

## Layout

This repo pins the two pieces as git submodules (local file-path URLs — see `.gitmodules`)
and adds demo programs:

```
cpp26-reflect-nanobind/
├── llvm-project/      submodule → ~/git/llvm-project  @ reflection-p2996
│                      The clang-p2996 fork (the compiler half). Its CLAUDE.md has
│                      build details. The usable toolchain is already installed at
│                      ~/llvm-toolchain (you rarely rebuild it).
├── nanobind/          submodule → ~/git/nanobind      @ mk-reflect
│                      The reflection-driven binder (the active development surface).
│                      include/nanobind/nb_reflect*.h + tests/test_reflect*. See its
│                      CLAUDE.md and docs/reflection.rst.
├── examples/          standalone reflection demos from this prove-out:
│                      refl.cpp           – minimal: print a struct's fields
│                      serialize_poc.cpp  – a complete reflection-driven binary+JSON
│                                           serializer (no per-type code, no macros)
└── CLAUDE.md          (this file)
```

Pinned submodule commits (the captured state): `llvm-project @ d4ae403`,
`nanobind @ e3d900b`. `git submodule status` shows the current pins.

## The idea, in one paragraph

`nb::reflect_<^^my_namespace>(m)` (in `nanobind/include/nanobind/nb_reflect.h`) walks a C++
namespace with reflection and emits ordinary `nb::class_/.def` bindings via splices inside
`template for` loops — classes, inheritance (incl. multiple-base flattening), methods/
operators/enums, with per-entity control via `[[=...]]` annotations
(`nb_reflect_annotations.h`: skip / rename / doc / return-value policy / keep-alive). The
only thing that can't be done in-language — a virtual-override **trampoline** — has a
text-**codegen fallback** (`nb_reflect_codegen.h`) that emits trampoline source for a
two-stage build. Full feature list + limitations: `nanobind/docs/reflection.rst` and
`nanobind/CLAUDE.md`.

## Environment (this exact laptop)

- **Toolchain**: `~/llvm-toolchain` — the from-source clang-p2996 (clang/clang++/lld + a
  libc++ that provides `<meta>`/`<experimental/meta>`), built from `llvm-project/`. Already
  built; use it directly. Native target AArch64.
- **Reflection flags**: `-std=c++26 -freflection-latest -stdlib=libc++`, plus (mandatory on
  macOS) `-isysroot "$(xcrun --show-sdk-path)"`. A from-source clang does not bake in the
  SDK path. `-freflection-latest` is the umbrella flag enabling P2996 + parameter reflection
  + expansion statements + annotations (P3394) + the rest.
- **Python**: Homebrew **`python3.12`** (`/opt/homebrew/bin/python3.12`). The system
  `/usr/bin/python3` lacks dev headers — do not use it.
- **Scratch build state (under `/tmp`, recreate if cleared)**: venv with pytest at
  `/tmp/nbvenv`; nanobind CMake build tree at `/tmp/nbbuild`.
- Tools: `cmake`, `ninja`, `ccache` via Homebrew.

## First-time setup on a fresh checkout of THIS repo

```bash
cd ~/git/cpp26-reflect-nanobind
# The submodule URLs are local paths on this laptop; this materializes them.
git -c protocol.file.allow=always submodule update --init --recursive
```
(If `~/llvm-toolchain` is missing, rebuild it per `llvm-project/CLAUDE.md` → "Full
reflection toolchain" build; otherwise skip — it's already there.)

## Build & test the binder (the main loop)

```bash
TC=~/llvm-toolchain
# venv + build tree (skip if /tmp/nbvenv and /tmp/nbbuild already exist)
/opt/homebrew/bin/python3.12 -m venv /tmp/nbvenv && /tmp/nbvenv/bin/pip -q install pytest
cmake -S nanobind -B /tmp/nbbuild -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=$TC/bin/clang -DCMAKE_CXX_COMPILER=$TC/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DPython_EXECUTABLE=/tmp/nbvenv/bin/python \
  -DNB_TEST=ON -DNB_TEST_FREE_THREADED=OFF -DNB_TEST_STABLE_ABI=OFF \
  -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath,$TC/lib" \
  -DCMAKE_MODULE_LINKER_FLAGS="-Wl,-rpath,$TC/lib"

ninja -C /tmp/nbbuild test_reflect_ext test_reflect_codegen_ext
DYLD_LIBRARY_PATH=$TC/lib PYTHONPATH=/tmp/nbbuild/tests \
  /tmp/nbvenv/bin/python -m pytest nanobind/tests/test_reflect.py \
  nanobind/tests/test_reflect_codegen.py -W error::RuntimeWarning
```
All reflection tests pass. (`test_reflect_codegen_ext` exercises the two-stage codegen:
CMake builds a generator, runs it to emit a trampoline header, then builds the module that
includes it.)

## Build & run the standalone demos

```bash
TC=~/llvm-toolchain
$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
  -L $TC/lib -Wl,-rpath,$TC/lib examples/serialize_poc.cpp -o examples/serialize_poc
./examples/serialize_poc          # JSON + binary round-trip, all reflection-driven
```
`examples/refl.cpp` builds the same way and prints a struct's fields.

## Status & roadmap

Implemented in the binder: classes/ctors/data/static/methods (+ overloads), full
function-type qualifier matching (`const`/`noexcept`/`&`; skips `volatile`/`&&`/variadic),
operators→dunders, enums, single + multiple inheritance (flattening), virtual overrides
(two-tier trampolines), and annotation-driven control (skip/rename/doc/rv_policy/
keep_alive). Remaining: templates (explicit instantiations, annotation-driven), keyword-arg
names (P3096), class/enum docstrings, per-arg ownership transfer. The binder's git history
on `mk-reflect` has one commit per feature; `nanobind/docs/reflection.rst` is the
user-facing reference.

## Gotchas (carried over; see submodule CLAUDE.md files for detail)

- **Spliced-lambda mangler crash** in clang-p2996: never put a spliced type
  (`[:type_of(x):]`) in a lambda *signature* passed to a dependent `cls.def*` call. The
  binder works around this throughout.
- **Annotation values must be valid template arguments** (no `const char*` members; strings
  use a `fixed_string<N>`).
- Everything needs the p2996 toolchain; both the generated trampolines and the binder use
  splices, so even generated code is compiled by `~/llvm-toolchain`.

## Working agreements

- Binder changes: edit in `nanobind/` on `mk-reflect`, push to its `fork` remote
  (`Cfretz244/nanobind`); never push to `origin` (upstream wjakob).
- After updating a submodule, `git add <submodule> && git commit` here to re-pin the state.
- `.gitmodules` uses local file-path URLs on purpose — this umbrella is a laptop snapshot,
  not a portable distribution.
