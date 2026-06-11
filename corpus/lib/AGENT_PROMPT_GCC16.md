# GCC 16 re-home wave protocol

You are a **GCC-16 corpus worker** for ONE run under `corpus/runs/<slug>/`.
The corpus is being re-homed from the bloomberg/clang-p2996 fork (macOS) onto
**stock GCC 16.1** (the `gcc16-reflect` docker container). Your run is fully
green on the clang backend (see its `result.json`); your job is to make the
SAME run green on the **gcc16 backend**: all lanes the run's meta.toml
declares (constexpr; plus emit + Gate 6b surface diff when `[emit]` is
enabled), three-way-validated against the same native oracle, with the result
recorded in `result-gcc16.json`.

## How to run things

Everything GCC executes INSIDE the container via the wrapper (repo mounted at
/work, cwd /work/corpus, `CORPUS_TOOLCHAIN=gcc16` preset):

```bash
corpus/lib/gcc16_run.sh python3 lib/run_gates.py runs/<slug>            # the full gate
corpus/lib/gcc16_run.sh python3 lib/run_gates.py runs/<slug> --mode emit  # one lane while debugging
corpus/lib/gcc16_run.sh bash -c '<anything>'                            # ad-hoc commands
```

The backend machinery is already ported (lib/env.sh and friends; gcc16 build
dirs are `binding/build-gcc16`, `binding/build-emit-gcc16`, `tests/build-gcc16`
— the macOS artifacts and `result.json` must NOT be touched). Useful facts:

- `run_gates.py` auto-translates `-fconstexpr-steps=N` in meta.toml
  `extra_cflags` to GCC's `-fconstexpr-ops-limit=N`. If your run needs a
  DIFFERENT budget (or other gcc-only flags), add a `[gcc16]` table:
  ```toml
  [gcc16]
  extra_cflags = ["-fconstexpr-ops-limit=800000000"]   # replaces the translated set
  ```
- Compiled-library runs: build the run's archive inside the container first —
  `corpus/lib/gcc16_run.sh bash lib/build_cmake_lib.sh <slug> <std>` (or
  `bash lib/build_abseil.sh` for the abseil_* runs). One g++ archive serves
  both lanes (`--prod` variants are a clang-backend concept; not needed).
  meta.toml `extra_libs` paths are rewritten to `build-gcc16/` mechanically.
- The nanobind support archive is prebuilt at `build-gcc16/prod/` (rebuild:
  `corpus/lib/gcc16_run.sh bash lib/build_nanobind_prod.sh --force`).
- The container has ~31 GB; heavy emit generators (json-scale) need several GB
  and minutes — stream long builds (`... 2>&1 | tee /tmp/<slug>_gate.log`) and
  run the build script directly before the gate when you expect >5 minutes.

## Success criteria (all of them)

1. `result-gcc16.json` outcome is **E** (or E-weak only if the run was E-weak
   on clang), every declared lane green, `surface=pass` when [emit] is on.
2. The bound API surface matches the clang lane's expectations: same tests,
   same `expected.json` semantics. The ONE sanctioned surface difference vs
   the historical record is the removed entity-proxy feature (private-base
   `using` re-exports no longer bind anywhere — abseil_statusor's tests were
   already regenerated for this).
3. The clang backend stays untouched: do not edit `result.json`, and any
   shared-source change keeps the clang lane green (see fix discipline).

## The divergence catalog (read before debugging ANYTHING)

Known clang-p2996 <-> GCC 16 divergences, from the spike + seed runs
(gcc16-proveout/HANDOFF.md has the full narrative):

1. **P3560 strictness**: GCC metafunctions THROW `std::meta::exception` on
   wrong-kind arguments where the fork was lenient. Gate with
   `is_type`/`is_function` first. "uncaught std::meta::exception" errors are
   often CASCADES of an earlier `[-Wtemplate-body]` parse-time warning — fix
   those first.
2. **Discarded `if constexpr` branches inside an expanded `template for` body
   are fully checked**. A splice valid only for the taken branch must live in
   an info-NTTP helper template (`reflect_class_of` pattern, nb_reflect.h).
3. **A lambda whose body splices an enclosing info NTTP cannot decay to a
   function pointer** (consteval-only under GCC). Hoist: `constexpr auto mp =
   &[:fn:];` then call through `mp`.
4. **GCC-1**: lifting an implicit special member's reflection into
   `define_static_array` instantiates its DEFINITION (hard error for
   `vector<unique_ptr<T>>`-member shapes). EVERY class-member lift must go
   through `liftable_class_members` — audit any walk you add.
5. **GCC-2**: a constexpr LOCAL is rejected as an expansion-statement range
   inside a template — hoist to a variable template (`emit_indices_v`).
6. **GCC-5**: a DEPENDENT noexcept-specifier left unresolved ICEs
   `nothrow_spec_p` when the spliced function type meets a partial-spec
   matrix or `is_noexcept(type)`. Fixed binder-wide via `nb_fn_type_of(fn)`
   (forces resolution; nb_reflect.h) — route any NEW decl-derived function
   type through it. Repro: gcc16-proveout/probes/xfail_gcc5_*.cpp.
7. **API drift is already shimmed** (`nb_annotations_of_type`,
   `nb_has_ellipsis_parameter`, `<meta>` vs `<experimental/meta>`,
   `NB_REFLECT_BIG_STEPS`) — use the shims in any new code.
8. **Annotation guards**: clang spells P3394 support
   `__has_feature(annotation_attributes)`; GCC needs
   `defined(__cpp_impl_reflection)`. Run fixtures with `[[=...]]` markers
   must use BOTH (see runs/json/binding/jsontest.h or the unit fixture's
   NB_FIXTURE_ANN). If every rename/skip fails at once, look here first.
9. **libc++-internal names in run fixtures** (`__char_traits_base` was the
   field case) won't exist on libstdc++ — replace with portable standard C++.
10. **GCC constant-evaluation budget**: default ~33M ops. The translated
   budgets have been enough so far; if you hit "constexpr evaluation
   operation count exceeds limit", raise via `[gcc16] extra_cflags`.

## Fix discipline

- Run-local files (`runs/<slug>/**` except `result.json`): edit freely.
  Fixture-header portability fixes (catalog items 8/9) are the COMMON case.
- **Binder/emitter fixes** (`nanobind/include/nanobind/nb_reflect*.h`):
  allowed for GENERAL bugs (a divergence shape above appearing at a new
  site, a spelling gap, a classifier divergence). Rules:
  - WHAT-to-bind logic in the shared classifiers (nb_reflect.h); text
    rendering in nb_reflect_emit.h / nb_reflect_spell.h. Never fork a
    decision into the emitter.
  - Portability fixes must keep clang-p2996 working: prefer compiler-neutral
    idioms over `#ifdef`; guard with `#if !defined(__clang__)` only as a
    last resort.
  - After ANY binder edit, rerun BOTH unit suites and note results:
    ```bash
    # GCC (container):
    corpus/lib/gcc16_run.sh bash -c 'ninja -C /work/gcc16-proveout/build-nanobind \
      test_reflect_ext test_reflect_emit_ext test_reflect_codegen_ext && \
      cd /work/nanobind/tests && PYTHONPATH=/work/gcc16-proveout/build-nanobind/tests \
      /work/gcc16-proveout/venv/bin/python -m pytest test_reflect.py \
      test_reflect_codegen.py test_reflect_emit.py -q -W error::RuntimeWarning'
    # clang (host):
    ninja -C build test_reflect_ext test_reflect_emit_ext test_reflect_codegen_ext && \
      DYLD_LIBRARY_PATH=$PWD/toolchain/lib PYTHONPATH=$PWD/build/tests \
      .venv/bin/python -m pytest nanobind/tests/test_reflect.py \
      nanobind/tests/test_reflect_codegen.py nanobind/tests/test_reflect_emit.py \
      -q -W error::RuntimeWarning
    ```
    Both must be 129/129 (plus any cases you added).
  - Add a fixture case + test when the bug class is representable in the
    unit suite (the GCC-5 `swap_with` shape is the model).
  - Write `runs/<slug>/findings_draft/BINDER-DRAFT-<n>.md` (dedup_key,
    symptom, root cause, fix summary, files touched).
- **GCC bugs** (ICEs, wrong-rejects, miscompiles): do NOT patch GCC. Minimize
  a repro into `gcc16-proveout/probes/xfail_gcc<N>_<slug-ish-name>.cpp`
  (numbered after the existing ones; header comment = expected vs actual +
  exact command), work around in the binder/run if possible, and write
  `runs/<slug>/findings_draft/GCC-DRAFT-<n>.md`. These get filed on
  gcc.gnu.org bugzilla later (Phase 4) — the repro quality is what matters.
- **Surface differences**: if a member binds on clang but cannot on GCC (or
  vice versa), that is a FINDING, not something to paper over with weakened
  tests. `surface_diff_ignore` in meta.toml only with a triaged
  justification recorded in the run's notes.
- NO git commits. NO edits outside `runs/<slug>/**`, `nanobind/include/`,
  `nanobind/tests/`, and `gcc16-proveout/probes/`.

## Report back (your final message)

- `result-gcc16.json` outcome + per-lane outcomes + surface status (quote the
  run_gates.py summary line).
- Every file changed OUTSIDE runs/<slug>/ with a one-line why, and the unit
  suite results (both compilers) if you touched the binder.
- findings_draft list with dedup_keys (so the supervisor can dedup across
  the wave).
- Anything papered over and why.
- Timings worth noting (a run that needs huge budgets or minutes-long
  compiles under GCC).
