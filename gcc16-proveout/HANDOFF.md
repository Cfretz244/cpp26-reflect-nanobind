# HANDOFF — re-homing the reflection binder onto GCC 16

Written 2026-06-11 at the end of the GCC 16 prove-out session. Audience: the
next agent, starting the actual migration. Read `FINDINGS.md` (same
directory) for the determination narrative; this file is the operational
handoff. The umbrella CLAUDE.md still describes the clang-p2996 world — it is
accurate for the clang lane and NOT yet updated for this migration.

## Mission

Make **GCC 16 the primary toolchain** for the binder (`nanobind/` submodule,
`nb_reflect*.h`) and its corpus, demoting the bloomberg/clang-p2996 fork to a
secondary/differential role (or retiring it). Two decisions are already made
by the user — do not relitigate:

1. **GCC 16 is the target.** It is a released compiler implementing the
   C++26-adopted reflection set (P2996R13, P1306, P3096, P3394, P3491,
   P3560) under `-std=c++26 -freflection`.
2. **The entity-proxy feature (using-redeclaration binding) is to be
   DROPPED**, not emulated. Verified NOT in C++26: P3687R1's poll 2b landed
   (`^^` on a using-declarator is ill-formed; entity proxies deferred to a
   future standard). The clang fork's `-fentity-proxy-reflection` is a
   post-C++26 reference-implementation extension and the user explicitly
   does not want to depend on it. Memory file `entity-proxy-not-cpp26`
   records this.

## State as of this handoff (all committed)

- **`nanobind` submodule, branch `gcc16-spike` (commit `380b9ec`)**: the
  portability spike. The ENTIRE reflection suite builds and runs under stock
  GCC 16.1: **129/131 pass** across all three lanes (constexpr `reflect_`,
  emit backend + surface diff, two-stage codegen trampolines). The only 2
  failures are `test38_private_base_using_reexports` (constexpr + emit
  parametrizations) — exactly the entity-proxy feature being dropped. The
  spike is fully backward-compatible: the clang-p2996 toolchain passes
  **131/131** with it (regression-run on the host this session).
  The submodule working tree is currently back on `mk-reflect`; the umbrella
  pin is unchanged. `gcc16-spike` is LOCAL-ONLY (not pushed to the fork).
- **Umbrella `gcc16-proveout/`** (commits `9e19f1a`, `1feb50e`, + this one):
  `Dockerfile`, feature probes (`probes/0*.cpp`, all passing), three
  verified GCC bug repros (`probes/xfail_gcc{1,2,3}_*.cpp`), `FINDINGS.md`,
  this file.
- **Docker**: base image `gcc:16` (gcc 16.1.0, aarch64) and derived
  `gcc16-reflect` (adds python3-dev/python3-venv/cmake/ninja/git) are built
  locally. `gcc16-proveout/build-nanobind/` is a configured GCC build tree
  (git-ignored); `gcc16-proveout/venv/` is a Linux venv created from inside
  the container (git-ignored; paths are container paths under `/work`).
- **Examples**: `examples/refl.cpp` and `examples/serialize_poc.cpp` run
  under GCC with only `<experimental/meta>` → `<meta>`. Not yet committed as
  source changes (the prove-out sed'd copies); making the examples portable
  (`__has_include` dance) is trivial follow-up.

## How to build & test (exact commands)

```bash
cd ~/git/cpp26-reflect-nanobind
(cd nanobind && git checkout gcc16-spike)          # the spike branch

# image (skip if `docker image ls gcc16-reflect` shows it):
docker build -t gcc16-reflect gcc16-proveout

# configure + build + test, all inside the container:
docker run --rm -v "$PWD":/work gcc16-reflect bash -c '
  python3 -m venv /work/gcc16-proveout/venv 2>/dev/null
  /work/gcc16-proveout/venv/bin/pip -q install pytest
  cmake -S /work/nanobind -B /work/gcc16-proveout/build-nanobind -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
    -DPython_EXECUTABLE=/work/gcc16-proveout/venv/bin/python \
    -DNB_TEST=ON -DNB_TEST_FREE_THREADED=OFF -DNB_TEST_STABLE_ABI=OFF
  ninja -C /work/gcc16-proveout/build-nanobind \
    test_reflect_ext test_reflect_emit_ext test_reflect_codegen_ext
  cd /work/nanobind/tests && \
    PYTHONPATH=/work/gcc16-proveout/build-nanobind/tests \
    /work/gcc16-proveout/venv/bin/python -m pytest \
      test_reflect.py test_reflect_codegen.py test_reflect_emit.py \
      -q -W error::RuntimeWarning'
# expected today: 129 passed, 2 failed (test38 constexpr+emit) -- the
# entity-proxy feature; goes to 131-equivalent green once it is removed.
```

CMake's detection prints `NB_HAS_REFLECTION_GCC - Success` and selects
`-std=c++26 -freflection` (`tests/CMakeLists.txt`, which already had the GCC
branch before this session; the spike added `NB_REFLECT_BIG_STEPS`).
Container venvs created in `/tmp` die with the container — keep the venv
under `/work/...` as above, and pass `-DPython_EXECUTABLE` again on any
re-`cmake`. The clang regression lane is unchanged (umbrella CLAUDE.md
"Build & test the binder"); run it after every shared-source change —
memory `corpus-regate-after-changes` exists for a reason.

Datapoint: GCC compiles `test_reflect.cpp` ~2x faster than the fork
(15.5s vs 29.5s on this machine).

## The complete clang-p2996 ↔ GCC 16 divergence catalog

Everything found porting ~5.4k lines of heavy reflection code. New code (and
the corpus port) will keep meeting these same shapes.

**Pure API drift (shimmed in `nb_reflect.h`; use the shims everywhere):**

| clang-p2996 | GCC 16 | shim |
|---|---|---|
| `annotations_of(r, ^^T)` (2-arg) | `annotations_of_with_type(r, ^^T)` | `nb_annotations_of_type` |
| `has_ellipsis_parameter(fn)` | `is_vararg_function(fn)` | `nb_has_ellipsis_parameter` |
| `<experimental/meta>` or `<meta>` | `<meta>` only | headers already use `__has_include(<meta>)` |
| `-freflection-latest -stdlib=libc++` | `-freflection` | CMake detection |
| `-fconstexpr-steps=N` | `-fconstexpr-ops-limit=N` | `NB_REFLECT_BIG_STEPS` |
| `__has_feature(annotation_attributes)` | `__cpp_impl_reflection` | fixture `NB_FIXTURE_ANN` |
| `is_entity_proxy` / `underlying_entity_of` / `-fentity-proxy-reflection` | **does not exist** | guarded `__has_feature(entity_proxy_reflection)`; feature being dropped |

Also: GCC has no `is_inline_namespace` (spell.h already works around its
absence — fork lacked it too). GCC's full `std::meta` surface is dumped in
the session log; everything else the binder uses exists under the same name.
`symbol_of(op)` returns the bare symbol (`"*"`) on both — the emit code's
`"operator" + symbol_of(op)` is correct as-is.

**Semantic divergences (idiom moves; patterns to follow in new code):**

1. **P3560 strictness**: GCC metafunctions THROW `std::meta::exception` on
   wrong-kind arguments (`is_class_type` on a function, `type_of` on a
   namespace, `annotations_of` on a template) where the fork returns
   false / is lenient. Always gate with `is_type`/`is_function` first. The
   binder was already disciplined about this; new corpus-port code must be
   too.
2. **Discarded `if constexpr` branches inside an expanded `template for`
   body are fully checked** (the loop variable is not a dependent entity).
   A splice that is only valid for the taken branch must live inside an
   info-NTTP helper template (`reflect_class_of` / `reflect_enum_of`
   pattern, nb_reflect.h ~2880). Probe: `probes/09_discarded_splice.cpp`
   (failing shape) and `09b_workaround.cpp` (portable shape).
3. **A lambda whose body splices an enclosing info NTTP is consteval-only**
   under GCC — it cannot decay to a function pointer. Hoist the splice:
   `constexpr auto mp = &[:fn:];` then use `(self.*mp)(...)` in the lambda
   (`reflect_bind_conversion` pattern). Generic method-binder lambdas were
   NOT affected (they don't decay); expect this only on concrete-signature
   capture-less lambdas. Repro: `probes/xfail_gcc3_*.cpp`.
4. **GCC-1 (hard GCC bug)**: lifting an implicitly-declared special member's
   reflection into `define_static_array` instantiates its DEFINITION —
   hard error for `vector<unique_ptr<T>>`-member shapes. EVERY class-member
   lift must go through `liftable_class_members` (nb_reflect.h ~1848; the
   spike routed all 4 sites in nb_reflect.h + 7 in nb_reflect_emit.h).
   Audit any walk you add. Repro: `probes/xfail_gcc1_*.cpp`.
5. **GCC-2 (GCC bug)**: a constexpr LOCAL is rejected as an
   expansion-statement range inside a template — hoist to a variable
   template (`emit_indices_v` pattern). Repro: `probes/xfail_gcc2_*.cpp`.
6. **Functions with consteval-only parameter types must be consteval** under
   GCC — the deliberately-undefined non-constexpr diagnostic function trick
   (`reflect_discovery_diverged`) is declared consteval there instead.
7. Non-transient constexpr allocation does not exist on either compiler:
   `constexpr auto v = <consteval fn returning std::vector>()` is an error —
   always lift through `define_static_array` (existing binder style).

**clang-only workarounds that GCC does NOT need** (leave them in for now,
they're harmless; candidates for simplification if the fork is retired):
spliced types in lambda SIGNATURES work fine on GCC (probe 06) — the
pointer-to-member/fixed-return-type contortions exist for the fork's mangler
crash; the TC-0004/0009 same-headed-sibling NTTP dispatch works out of the
box on GCC (probe 05); deduction-guide stripping (TC-0008) is moot on GCC
but `namespace_members_for_binding` doing it is fine.

## Work plan

**Phase 0 — land the spike (small).**
Re-run the clang lane green (done this session, re-verify), merge
`gcc16-spike` → `mk-reflect`, push to the fork (`Cfretz244/nanobind`),
re-pin the umbrella. Make the two examples' include portable.

**Phase 1 — remove the entity-proxy feature (decided).**
On `mk-reflect`: stop passing `-fentity-proxy-reflection` in
`tests/CMakeLists.txt`; delete/neuter the proxy paths (`is_using_proxy`,
`proxy_underlying`, `classify_proxy`, `reflect_bind_proxy`,
`class_member_kind::proxy`, the emit `append_proxy`/`proxy_route` mirror,
and the `__has_feature(entity_proxy_reflection)` block); retire test38 and
the `proxy_test` fixture namespace; update `docs/reflection.rst` +
`nanobind/CLAUDE.md` (using-redeclarations: private-base re-exports do not
bind — public-base ones are already covered by inheritance/flattening).
Expect the abseil/StatusOr corpus surface to shrink (`value()` via private
base) — its expectations need regenerating. After this, GCC and clang lanes
should both be 100% green with IDENTICAL surfaces.

**Phase 2 — corpus under GCC (the real test; this decides "sole
toolchain").**
The corpus (`corpus/`, 36 runs, three-way validation per
`corpus/lib/run_gates.py`) currently assumes the macOS/clang world: the
oracle + constexpr + generator TUs build with the fork, and the emit lane's
"production compiler" is Apple Clang (`corpus/lib/build_nanobind_prod.sh`).
The GCC re-home: generator TUs build with `g++ -freflection` in the
container; the emit lane's production compiler is plain `g++` (no reflection
flags) — a nice simplification since it's one compiler. Per-run library
deps (glm/json/eigen/abseil/...) must be fetched/built inside the container.
Sweep in waves like the emit migration did (`AGENT_PROMPT_EMIT.md` is the
protocol precedent). Watch for: `-fconstexpr-ops-limit` headroom on the big
walks (eigen/json needed raised budgets on clang; GCC's default is ~33M
ops), GCC-native ICEs at scale (none seen yet, but the suite is much
smaller than eigen), and divergence shapes 1–5 above in run-specific glue.
Budget expectation: drift fixes so far averaged ~30 minutes each with the
error-category triage loop (`ninja ... 2>&1 | grep -E "error" | sort |
uniq -c`); expect a handful more at corpus scale, not dozens.

**Phase 3 — decide the fork's fate.**
Options: (a) keep clang-p2996 as a differential second implementation
(catches real bugs — this session caught GCC-1 only because the clang lane
defined correct behavior); (b) retire it and delete the toolchain build
machinery from the umbrella (CLAUDE.md "Build the toolchain", TC-XXXX local
fixes become history). Recommendation in FINDINGS.md: keep it secondary
until the corpus is green on GCC, then ask the user.

**Phase 4 — file the GCC bugs upstream** (gcc.gnu.org bugzilla, component
c++; CC the reflection implementers). GCC-1 and GCC-2 are clear-cut with
in-repo repros; GCC-3/GCC-4 (discarded-branch checking, probe 09) are
divergence questions worth filing as such. Mirror the TC-XXXX discipline:
one `UPSTREAM.md` per finding once filed (the umbrella working agreement,
adapted from the clang flow).

## Traps for the next agent

- `liftable_class_members` for EVERY new class-member lift (GCC-1 bites as
  a hard error in unrelated-looking code; the vector<unique_ptr> shape is
  everywhere in real libraries).
- The fixture (`test_reflect_fixture.h`) is shared by every backend; its
  `NB_FIXTURE_ANN` now keys off `__cpp_impl_reflection` — if annotations
  silently vanish again (all renames/skips failing at once), look there
  first.
- The GCC build tree caches `Python_EXECUTABLE`; if cmake re-runs and the
  venv moved/died you get "Cannot run the interpreter" — recreate the venv
  at `gcc16-proveout/venv` and re-pass the flag.
- Do not push `gcc16-spike` to upstream wjakob (`nanobind` submodule pushes
  go to the `Cfretz244/nanobind` fork only — standing working agreement).
- Error triage at scale: GCC template-body diagnostics (`[-Wtemplate-body]`)
  surface drift at PARSE time inside uninstantiated templates — fix those
  first; later "uncaught std::meta::exception" / misclassification errors
  are often cascades of them.
- The probes in `gcc16-proveout/probes/` double as a quick conformance
  smoke for new GCC point releases: `0*.cpp` must all pass, `xfail_*` are
  expected to fail until GCC fixes them (recheck on every GCC upgrade).
