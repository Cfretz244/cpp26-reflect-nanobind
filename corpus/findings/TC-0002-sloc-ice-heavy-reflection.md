# TC-0002 — clang-p2996 ICE (SourceManager "Invalid SLocOffset") under very heavy reflection

- **Status:** suspected (toolchain ICE) + a constexpr-step-budget scale wall
- **Kind:** internal compiler error / assertion failure
- **Signature:** `Assertion failed: (0 && "Invalid SLocOffset or bad function choice"), function getFileIDLoaded, SourceManager.cpp:876`
- **Toolchain:** `llvm-project` @ `d4ae403` (clang-p2996), bundled libc++
- **Found via:** Phase 1, nlohmann/json — binding `^^nlohmann::json` (= `basic_json<>`).

## Two stacked symptoms

Reflecting the whole `basic_json` value type drives an enormous amount of constexpr work (its
user-spec discovery fixpoint walks ~hundreds of members plus json's internal template web —
`iter_impl`, `json_pointer`, `json_sax`, `detail::*`). Two failure modes result:

1. **Default flags — constexpr step-budget wall (clean diagnostic).** The binder's
   `required_user_specs` / STL walks exceed the default `-fconstexpr-steps`:
   ```
   error: call to consteval function 'emit_trampolines<...>' is not a constant expression
   note: constexpr evaluation hit maximum step limit; possible infinite loop?
   ```
   This is the standard-build outcome (corpus json run = **B.gen_compile**). Not itself a compiler
   bug — `basic_json` is genuinely huge — but see below.

2. **Raised `-fconstexpr-steps` — compiler ICE.** Bumping the limit so evaluation can proceed
   (`-fconstexpr-steps=200000000`) runs ~32 s and then **crashes the compiler**:
   ```
   Assertion failed: (0 && "Invalid SLocOffset or bad function choice"),
   function getFileIDLoaded, file SourceManager.cpp, line 876.
   ```
   A compiler must never assert/crash — it should diagnose. The "Invalid SLocOffset" suggests
   **source-location space exhaustion** from the volume of `std::meta` / `std::define_static_string`
   operations the deep reflection performs. This is the genuine toolchain bug.

## Reproducer

`corpus/runs/json/binding/gen.cpp` (+ `jsontest.h`) compiled against nlohmann/json v3.11.3:

```
TC=.../toolchain
$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
  -fconstexpr-steps=200000000 \
  -I corpus/libs/json/single_include -I corpus/runs/json/binding -I nanobind/include \
  corpus/runs/json/binding/gen.cpp -o /tmp/gen      # -> ICE after ~32s
```

Not yet minimized (needs nanobind + json). TODO before upstreaming: reduce to a standalone TU
that performs N `std::define_static_string` / `std::meta::members_of` ops in one consteval call
until SLoc space is exhausted, to confirm the cause is independent of nanobind/json.

## Assessment

`nlohmann::json` whole-`basic_json` binding is **intractable on this toolchain**: too big for the
constexpr budget, and the compiler ICEs when pushed past it. Recorded as the campaign's first
"heavy real-world value type" ceiling datapoint. The binder improvements extracted along the way
(BINDER-0004) reduce — but do not eliminate — the constexpr cost; the residual wall is `basic_json`'s
intrinsic size plus the SLoc-exhaustion ICE.

dedup_key: `sloc-ice-heavy-reflection`.
