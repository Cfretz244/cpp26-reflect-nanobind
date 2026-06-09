# corpus — binding-generator correctness campaign

Tests the C++26-reflection→nanobind binding generator against real C++ libraries. Each library
is a pinned git submodule under `libs/<slug>`; its binding code + custom-fit correctness tests
live under `runs/<slug>` and are committed. One agent scaffolds one library; results aggregate
across all of them and feed binder/toolchain fixes.

## Layout

```
corpus/
├── manifest.toml          # library list + status ledger
├── result_schema.json     # shape of each runs/<slug>/result.json
├── lib/                   # reusable machinery (shared by every run)
│   ├── env.sh             # toolchain/python/flag resolution (source-able, bash+zsh)
│   ├── probe.sh           # Gate 1: -fsyntax-only include probe
│   ├── build_module.sh    # Gate 4 single-stage: binding.cpp -> .so (prebuilt nanobind static lib)
│   ├── build_module_codegen.sh  # Gate 4 two-stage: gen.cpp -> trampolines.gen.h -> module
│   ├── build_native.sh    # native oracle build (-O2; see TC-0001)
│   ├── run_gates.py       # per-run orchestrator: runs gates, writes result.json
│   └── aggregate.py       # roll up all result.json -> aggregate/{report,toolchain_bugs}.md
├── libs/<slug>/           # the library, as a pinned submodule
├── runs/<slug>/           # per-library, committed: meta.toml binding/ tests/ result.json
├── findings/              # binder + toolchain bug write-ups (TC-*, BINDER-*)
└── aggregate/             # report.md, results.jsonl, toolchain_bugs.md (generated)
```

## Per-repo workflow (Gates 0–6)

1. **Gate 0 — acquire & pin:** `git submodule add <url> corpus/libs/<slug>`, checkout a tag.
2. **Gate 1 — probe:** create `runs/<slug>/probe/probe.cpp` (just `#include` the public headers);
   `bash lib/probe.sh runs/<slug>/probe/probe.cpp -I libs/<slug>`. Fail ⇒ outcome **A** (stop).
3. **Gate 2 — subset:** pick a tractable binding surface (concrete classes/specializations, not the
   whole namespace if huge). Record `reflect_args` + rationale in `meta.toml`.
4. **Gate 3 — strategy:** virtuals you want overridable ⇒ `two_stage`, else `single_stage`.
5. **Gate 4–6 — compile / import / correctness:** write `binding/binding.cpp` (+ `binding/gen.cpp`
   for two_stage), a native `tests/oracle_native.cpp`, and `tests/test_bindings.py` (differential
   vs the oracle + invariants). Then run everything via the orchestrator:

```bash
python corpus/lib/run_gates.py corpus/runs/<slug>      # writes runs/<slug>/result.json
python corpus/lib/aggregate.py                          # refresh aggregate/*
```

`meta.toml` keys: `slug tier header_only url pin include_root main_headers module_name strategy
reflect_args subset_rationale skipped_features` (see any existing run for an example).

## Outcomes (failure taxonomy)

`A` won't compile under toolchain · `B` binding fails to compile · `C` import crashes ·
`D` behavior wrong · `E` clean success (≥1 differential/doc assertion) · `E-weak` import/invariant
only. A toolchain crash/ICE/wrong-rejection is recorded orthogonally in the `toolchain_bug` block
and written up under `findings/` for upstreaming.

## Environment

Self-contained in the umbrella repo: `../toolchain` (clang-p2996), `../build/tests/
libnanobind-static.a` (prebuilt), `../.venv` (python3.12 + pytest). The build helpers reuse the
prebuilt static lib — no per-run nanobind rebuild. See `../CLAUDE.md` for the canonical flags.

## Known gotchas baked into the helpers

- **TC-0001:** native oracles must build at `-O2` (at `-O0` this toolchain emits an unresolved weak
  `operator new[](size_t, std::__type_descriptor_t)` and the executable aborts at load).
- Binder bug **BINDER-0001** (templated members hard-erroring) was found+fixed during bring-up;
  the binder now gracefully skips member/constructor templates.
