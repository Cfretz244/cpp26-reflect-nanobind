# GCC 16 prove-out — findings & determination (2026-06-11)

## Question

Can GCC 16 (the first *shipping* compiler with C++26 reflection) become the
sole toolchain target for the reflection binder in the immediate term, or do
we still need the experimental bloomberg/clang-p2996 fork?

## Determination

**GCC 16 can host the binder today — with one feature gap — but it is not yet
a complete replacement.**

- The **entire reflection test suite passes under stock GCC 16.1** (Docker
  `gcc:16`, aarch64) after a small, fully backward-compatible porting spike:
  **129/131 pass** across all three lanes (constexpr `reflect_`, the emit
  backend incl. the surface diff, and the two-stage codegen trampolines).
- The **2 failures are one feature: using-redeclaration binding (entity
  proxies, BINDER-0009 / test38)**. That feature is built on the clang fork's
  `-fentity-proxy-reflection` extension (`using`-shadow enumeration), which
  has **no GCC equivalent** — it is not in C++26. Everything else the binder
  does is standard-reflection-expressible and works on GCC.
- The same spike still passes **131/131 on the clang-p2996 toolchain** (host
  macOS regression run) — the port is shims, not a fork.
- GCC compiled the main test TU **~2× faster** than the fork (15.5s vs 29.5s
  for `test_reflect.cpp`).

Recommendation: treat **GCC 16 as the primary target going forward** (it is a
released, supported compiler; the fork is frozen research). Keep the
clang-p2996 toolchain only for (a) the using-redeclaration feature until it's
either dropped, worked around per-class, or standardized (P3680-era shadow
enumeration), and (b) as the second implementation for differential testing.
"Sole toolchain" is **not yet** justified: the corpus (36 runs) has not been
swept under GCC, and GCC has its own reflection bugs (one hard one found in
this spike alone, see GCC-1) — the corpus sweep is the follow-up that would
settle it.

## What was run

- `examples/refl.cpp`, `examples/serialize_poc.cpp`: pass unmodified except
  `<experimental/meta>` → `<meta>` (GCC ships `<meta>` only).
- 9 feature probes (`probes/*.cpp`): core queries, nested `template for`,
  annotations + `extract` round-trip, P3096 parameter names +
  `has_default_argument`, `substitute`/`can_substitute`/reflection-NTTPs,
  splices in lambda bodies AND lambda signatures, consteval string rendering
  (`define_static_string`, `display_string_of`, `symbol_of`). All pass.
  Notably the TC-0004/TC-0009 same-headed-sibling dispatch shape and the
  spliced-type-in-lambda-signature shape (clang mangler crashes) work
  out of the box on GCC.
- The full nanobind reflection build (`tests/CMakeLists.txt` already had a
  GCC flag branch) + pytest suite, both backends, in the `gcc16-reflect`
  image (Dockerfile here; gcc:16 + python3-dev + cmake + ninja).

## The port (branch `gcc16-spike` in the nanobind submodule)

All changes are `#if defined(__clang__)`-style shims or idiom moves that are
valid on both compilers; clang suite unaffected.

1. **API drift shims** (`nb_reflect.h`, used by all headers):
   `annotations_of(r, type)` → GCC `annotations_of_with_type(r, type)`;
   `has_ellipsis_parameter` → GCC `is_vararg_function`.
   (`nb_annotations_of_type` / `nb_has_ellipsis_parameter` wrappers.)
2. **Flags**: GCC wants `-freflection` (no `-fexpansion-statements`, no
   `-stdlib` change) and `-fconstexpr-ops-limit=N` instead of
   `-fconstexpr-steps=N` (`NB_REFLECT_BIG_STEPS` in tests/CMakeLists.txt).
3. **Fixture annotations**: `NB_FIXTURE_ANN` keyed off
   `__has_feature(annotation_attributes)` (clang-only, silently 0 on GCC →
   every skip/rename/doc vanished); now also keys off
   `__cpp_impl_reflection`.
4. **Dispatch forwarders** (`reflect_class_of`/`reflect_enum_of`): GCC checks
   splices in DISCARDED `if constexpr` branches inside an expanded
   `template for` body (the loop variable is not dependent); a function/
   namespace reflection is "not usable in a splice type" there. Splices moved
   into info-NTTP helper templates where they stay dependent until taken.
5. **Conversion-operator lambdas**: GCC treats a lambda whose body splices an
   info NTTP as consteval-only — it cannot decay to the function pointer
   nanobind wants. Splice hoisted to a `constexpr auto mp = &[:fn:]`
   pointer-to-member outside the lambda.
6. **`liftable_class_members`** (the big one, GCC-1 below): implicit
   copy/move ctors + assignment operators are dropped from every
   class-member `define_static_array` lift (they are never bound; the
   implicit default ctor, which `init<>` consumes, is kept).
7. **Divergence diagnostic**: GCC rejects a non-consteval function declared
   with an `info` parameter ("function of consteval-only type must be
   declared consteval"), so `reflect_discovery_diverged` is consteval (and
   still undefined) under GCC.
8. **`emit_indices_v` variable template**: GCC does not accept a constexpr
   LOCAL as an expansion-statement range inside a template ("not a constant
   expression"); hoisted.

## GCC 16.1 reflection bugs / divergences found (candidates to file upstream)

- **GCC-1 (real bug, hard error)**: lifting the reflection of an
  IMPLICITLY-declared special member into `define_static_array` (i.e. using
  it as an NTTP) instantiates that member's DEFINITION. For a class with a
  `std::vector<std::unique_ptr<T>>` member (copyable by declaration,
  ill-formed to instantiate) the lift itself hard-errors. 25-line repro in
  probes/ (p15-p18 bisection: `members_of` alone is fine; the lift triggers).
- **GCC-2 (likely bug)**: a constexpr local variable is rejected as the range
  of an expansion statement inside a function template ("not a constant
  expression"); the same initializer as a variable template works.
- **GCC-3 (divergence, arguably GCC's reading of consteval-only
  propagation)**: a lambda whose body contains a splice of an enclosing
  info NTTP is consteval-only → its function-pointer conversion is an
  immediate function ("immediate evaluation returns address of immediate
  function").
- **GCC-4 (divergence, possibly conformant)**: discarded `if constexpr`
  branches inside expanded `template for` bodies are fully semantically
  checked (clang-p2996 does not check them). Core-issue territory; the
  forwarder idiom is portable either way.
- **P3560 semantics**: GCC's metafunctions THROW `std::meta::exception` on
  wrong-kind arguments (`is_class_type` on a function, `type_of` on a
  namespace) where the fork returns false / is lenient. The binder's
  existing is_type/is_function gating was almost everywhere correct already.

## Feature-support snapshot (GCC 16.1, `-std=c++26 -freflection`)

| Needed by binder | GCC 16 |
|---|---|
| P2996 core (members_of, splices, NTTPs, substitute…) | yes |
| P1306 `template for` | yes (no extra flag) |
| P3394 annotations | yes (incl. `annotations_of_with_type`) |
| P3096 parameter names / `has_default_argument` | yes |
| P3491 `define_static_{string,array}` | yes |
| P3560 exception-based error handling | yes (stricter than fork) |
| `-fentity-proxy-reflection` (using-shadow enumeration) | **no — fork extension, no equivalent** |

## Follow-up (if pursued)

1. Sweep the corpus (36 runs) under GCC in the container — the real
   robustness test (Eigen/json-scale consteval walks, `-fconstexpr-ops-limit`
   headroom, GCC-native ICEs).
2. Decide the using-redeclaration story: drop the feature on GCC, or emulate
   (per-class `using`-target scan via bases_of + name lookup is NOT
   expressible in P2996 final for private bases).
3. File GCC-1/GCC-2 (and minimized GCC-3/4 questions) upstream.
4. Merge `gcc16-spike` → `mk-reflect` after a full clang corpus re-gate
   (memory: corpus-regate-after-changes).
