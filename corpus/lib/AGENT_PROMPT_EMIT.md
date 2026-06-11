# Emit-lane wave protocol (E2+)

You are an **emit-lane worker** for one corpus run. The constexpr lane of your
run is GREEN (outcome E) and must stay green; your job is to bring up the
**emit lane** -- the same bindings as generated source compiled by the
PRODUCTION compiler (Apple Clang + system libc++) -- and make the THREE-WAY
validation pass: oracle, constexpr module, emit module all agreeing, plus the
Gate 6b surface diff.

Unlike the Phase-3 protocol (AGENT_PROMPT.md), you MAY fix the binder/emitter
(`nanobind/` on `mk-reflect`) when you find a general bug -- with the
discipline below. A Fable reviewer will audit your diff before it lands; you
iterate with that reviewer until approval.

## Steps

1. **Three-file refactor** of `corpus/runs/<slug>/binding/` (bind-set defined ONCE):
   - `binding_includes.h` -- plain C++ BOTH compilers can parse: library +
     fixture headers + config defines. No `^^`, no `[[=...]]` (macro-guard any
     fixture annotations on `__has_feature(annotation_attributes)`).
   - `binding_args.h` -- P2996-only: `#include "binding_includes.h"`, any
     consteval marker helpers, and `#define CORPUS_REFLECT_ARGS ...` (the
     exact pack `binding.cpp` passed to `nb::reflect_`). If the constexpr run
     is `two_stage` (trampolines), append `^^nb::trampoline_all_` to the pack
     -- the emit lane inlines the same trampolines; the constexpr lane treats
     the marker as inert configuration.
   - `binding.cpp` shrinks to: nb_reflect.h + `binding_args.h` + its existing
     lane-specific includes (STL caster headers, a two_stage run's
     `trampolines.gen.h`) + `NB_MODULE(<mod>, m) { nb::reflect_<CORPUS_REFLECT_ARGS>(m); }`.
   - `gen_emit.cpp` (new):
     ```cpp
     #include <nanobind/nb_reflect_emit.h>
     #include "binding_args.h"
     int main(int argc, char** argv) {
         return nanobind::write_bindings(argv[1],
             nanobind::emit_bindings<CORPUS_REFLECT_ARGS>(
                 "<module_name>", "#include \"binding_includes.h\"\n")) ? 0 : 1;
     }
     ```
     The preamble path is resolved by `-include`/`-I` flags the harness passes;
     use the include form that matches how binding.cpp includes the library.
2. **meta.toml**: add
   ```toml
   [emit]
   enabled = true
   # std = "c++20"                  # only if the library needs another -std
   # extra_cflags = [...]           # stage-2 overrides; default = constexpr
   #                                #   extra_cflags minus reflection flags
   # surface_diff_ignore = [...]    # ONLY with a triaged justification in notes
   ```
3. **Iterate** `python corpus/lib/run_gates.py corpus/runs/<slug>` until the
   combined outcome is E (all three legs + surface). `--mode emit` re-runs
   just your lane while debugging; always finish with a full (default) run.
4. **Verify the constexpr lane is untouched**: its lane outcome must equal the
   pre-wave result.json's.

## Fix discipline (differs from Phase 3)

- Run-local files (`runs/<slug>/**`): edit freely.
- **Binder/emitter fixes** (`nanobind/include/nanobind/nb_reflect*.h`): allowed
  when the failure is a GENERAL bug (type-spelling gap, emission pattern bug,
  missing caster include, classifier divergence). Rules:
  - The fix must live at the right layer: WHAT-to-bind logic goes in the shared
    classifiers (nb_reflect.h), text rendering in nb_reflect_emit.h /
    nb_reflect_spell.h. NEVER fork a decision into the emitter.
  - Add/extend a unit static_assert or fixture case in nanobind/tests when the
    bug class is representable there.
  - After ANY binder edit: rebuild + rerun the nanobind reflection suite
    (test_reflect.py, test_reflect_codegen.py, test_reflect_emit.py) AND your
    run's full three-way gates. Note the suite result in your findings draft.
  - Write `runs/<slug>/findings_draft/BINDER-DRAFT-<n>.md` with a `dedup_key`,
    symptom, root cause, fix summary, and files touched.
- **Toolchain bugs** (clang-p2996 ICEs/miscompiles): do NOT fix the toolchain.
  Minimize a repro into `runs/<slug>/findings_draft/TC-DRAFT-<n>.md` +
  `repro.cpp`, work around in the run if possible, and report.
- NO git commits; NO edits outside `runs/<slug>/**` and `nanobind/` as scoped
  above; manifest.toml is read-only.

## Reviewer handoff

When green (or blocked), produce a summary for the Fable reviewer:
- the run's final lane outcomes + surface status,
- every file you changed OUTSIDE runs/<slug>/ with a one-line why,
- findings_draft list with dedup_keys,
- anything you papered over (surface_diff_ignore, skipped features) and why.
The reviewer checks: classifier/emitter layering, no weakened tests or diffs,
constexpr lane untouched, fix generality. Address their feedback and rerun the
gates until they approve.
