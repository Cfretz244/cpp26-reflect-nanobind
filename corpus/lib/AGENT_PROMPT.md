# Per-repo corpus agent prompt (Gates 0–6)

You are scaffolding ONE library of the binding-correctness corpus in
`~/git/cpp26-reflect-nanobind`. The dispatcher gives you: `slug`, `tier`, `url`,
`pin` (a release tag, or "latest release tag"), `include_root`. Every repo gets this
identical prompt, so results stay comparable; only the parameters differ.

Read first (mandatory): the repo-root `CLAUDE.md`, `corpus/README.md`, and ONE
existing run as a template — `corpus/runs/spdlog` for a header-only library,
`corpus/runs/abseil_status` for a compiled one.

## Procedure

- **Gate 0:** `git submodule add <url> corpus/libs/<slug>`, then checkout the pin
  (if told "latest release tag", list tags and pick the newest release).
- **Gate 1:** write `corpus/runs/<slug>/probe/probe.cpp` (just `#include` the public
  headers + `int main(){}`); run `bash corpus/lib/probe.sh <probe.cpp> -I <include
  root>`. If it fails: still write `meta.toml`, run the orchestrator (it records
  outcome **A**), and stop.
- **Gate 2 (the real judgment call):** pick the binding subset. **Bind the library's
  OWN API head-on** — its real concrete classes, real class-template
  specializations (listed explicitly in the `reflect_<...>` pack when not
  signature-reachable), real enums, real free functions. A small fixture namespace
  (a header in `runs/<slug>/binding/`) is allowed ONLY for what the real API cannot
  express to Python: factories for types with no bindable constructors,
  observability for effects the tests must see, thin wrappers over variadic or
  all-template fronts. A run whose tests exercise only fixture wrappers is a failure
  even if it reports E.
- **Gate 3:** `strategy = "single_stage"` unless the tests NEED Python-side virtual
  overrides (then `two_stage`; template: `corpus/runs/_fixture_virtual`).
- **Gates 4–6:** write `binding/binding.cpp`, `tests/oracle_native.cpp`,
  `tests/test_bindings.py`; iterate with
  `.venv/bin/python corpus/lib/run_gates.py corpus/runs/<slug>` until it reports an
  outcome. **E requires a genuine Layer-1 differential:** `oracle_native.cpp` drives
  the SAME scenario through the library natively and prints ONE JSON object on
  stdout; `test_bindings.py` loads `tests/expected.json` and asserts equality on real
  behavior (computed values, formatted output, ordering, error paths), plus Layer-3
  invariants (surface present, inheritance/isinstance, exception types). Import-only
  or hasattr-only assertions are not an acceptable core.

## Parallel dispatch (Phase 3 fan-out)

When the dispatcher says "Gate 0 already done", you are one of several sibling agents
running CONCURRENTLY in this same working tree. Three deltas apply:

- **Skip Gate 0.** The driver already added the submodule at the pin and registered the
  manifest entry (concurrent `git submodule add` races on `.gitmodules` and the index).
  Start at Gate 1. Never run a git command that mutates state (add/commit/checkout/
  submodule); read-only git (status/log/show) is fine.
- **Touch NOTHING outside `corpus/runs/<slug>/**`.** This was always the rule; under
  parallel dispatch it is load-bearing — another agent's run dir, `corpus/findings/`,
  and `corpus/manifest.toml` are all off-limits, no exceptions.
- **Draft findings go INSIDE your run dir**, as `corpus/runs/<slug>/findings_draft/*.md`
  (this replaces the `corpus/findings/LIB-*.md` draft path above). Do NOT assign
  TC-/BINDER-/LIB- numbers — parallel agents would collide; the driver assigns canonical
  numbers at triage. Each draft must carry a header line `dedup_key: <stable-slug>`
  (name the root-cause signature, not your library — e.g.
  `mangler-ice-spliced-lambda-signature`, not `mylib-crash`) so the driver can cluster
  the same bug found by several agents, plus the smallest trigger you isolated and the
  first diagnostics.

## Guardrails

- Modify ONLY `corpus/libs/<slug>` (the submodule add), `corpus/runs/<slug>/**`,
  and — if you find a genuine library-side bug — a draft `corpus/findings/LIB-*.md`.
  NEVER edit `nanobind/`, `llvm-project/`, `corpus/lib/`, `corpus/manifest.toml`,
  other runs, or the library's source.
- If you hit what looks like a BINDER or TOOLCHAIN bug (compiler crash/ICE, a
  static_assert out of `reflect.h`, behavior divergence traceable to the binding
  layer rather than your code): do NOT fix the binder or toolchain, and do NOT
  weaken the tests around it. First try a smaller legitimate subset (dropping the
  triggering entity) if a meaningful run survives; record what you dropped in
  `skipped_features`. If the run cannot proceed, let `run_gates.py` record the
  B/C/D outcome and write a draft `corpus/findings/` note with the smallest trigger
  you can isolate. Failure outcomes are valid results.
- Do not `git commit` anything; leave the working tree for the driver to review.
- Use exactly the repo-local environment (`./toolchain`, `.venv`, helpers in
  `corpus/lib/`) per `corpus/README.md`; install nothing; never touch `~/` paths.
- `meta.toml` keys must match the existing runs (see `corpus/README.md`); record
  honest `skipped_features` and a `subset_rationale` that names what is bound and
  what the differential checks.

## Done

Finish only when `run_gates.py` has written `corpus/runs/<slug>/result.json`. Report
back: the outcome, what you bound (and what you deliberately did not), the shape of
the differential, and any suspected binder/toolchain/library bugs with their
diagnostics.
