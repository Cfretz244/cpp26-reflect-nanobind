# PROVE_OUT.md — the reflection→nanobind binding prove-out: intended path

Durable roadmap for validating the C++26-reflection-driven nanobind binder against
real open-source C++ libraries. This is the committed companion to the (local,
non-committed) planning artifact under `~/.claude/plans/`; it is the source of truth
for **where we are and where we're going**. See also: `CLAUDE.md` (environment +
toolchain build), `corpus/README.md` (machinery/layout), `corpus/findings/`
(per-bug write-ups), `corpus/manifest.toml` (the run ledger + queue).

## Goal

`nb::reflect_<^^ns>(m)` auto-generates Python bindings from C++ via P2996 static
reflection. The prove-out validates binding **correctness** against real libraries —
ultimately hundreds, each scaffolded by its own agent that writes custom-fit
differential tests — by building a **reproducible, pinned test corpus**. Two
first-class deliverables:

1. **Binder coverage** — what real code does to the binder, fed back as fixes.
2. **Toolchain bugs** — the bleeding-edge clang-p2996 + libc++ toolchain is itself a
   primary subject; crashes / wrong-rejections / ABI gaps are surfaced, minimized,
   and fixed at the source (then re-pinned via the `llvm-project` submodule).

## The gate pipeline (per library)

Each library runs a strict, cheap-fail gate ladder; the run stops at the furthest
gate reached and records a structured `result.json`. Outcome taxonomy:

| Outcome | Meaning |
|---|---|
| **A** | won't compile under the toolchain (probe gate) |
| **B** | binding fails to compile (`B.gen`/`B.module`/`B.link`/`B.static_assert`) |
| **C** | import crashes (segfault / ImportError / missing symbol) |
| **D** | behavior wrong (differential mismatch) |
| **E** | success with ≥1 Layer-1/2 behavioral assertion |
| **E-weak** | success but invariants/import-only (no native-oracle differential) |

Gates: **0** acquire+pin (submodule) · **1** header-compile probe · **2** subset
selection · **3** single- vs two-stage strategy · **4** compile the binding · **5**
import smoke test · **6** correctness (full differential vs a native C++ oracle).
Mechanics live in `corpus/lib/` (`run_gates.py`, `build_module*.sh`,
`build_native.sh`, `aggregate.py`).

## Incremental ramp (tiers)

A tier is "ramped past" only when **≥1 library in it reaches `E`** (not `E-weak`).

| Tier | Theme | Libraries | Status |
|---|---|---|---|
| 0 | pipeline bring-up; data/ctors/by-value/free-ops | `_fixture_pod`, `_fixture_recursive`, **linalg** v2.2 | ✅ E |
| 1 | value types + operators + enums + STL casters | **glm** 1.0.1, **nlohmann/json** v3.11.3 | ✅ E |
| 2 | free-function / format-heavy; kwargs; member-fn-template gap | **fmt** 11.2.0 (`FMT_HEADER_ONLY`) | ✅ E |
| 3 | inheritance; shared_ptr; log-level enums | **spdlog**; (`tl::expected`) | ⏭️ **next** |
| 4 | virtual override → two-stage codegen | `_fixture_virtual` | ✅ E |
| 5 | stress ceiling; expression templates | **Eigen** (`Matrix<double,3,3>`); a header-only Boost piece | ⬜ frontier |
| ★ | **special case — breadth of fully-specialized concrete data structures** | **Abseil** 20250814.2 (`InlinedVector<int,4>`+`<double,2>` ✅; `flat_hash_map`/`FixedArray`/`int128` queued) | ✅ E |

Concrete queue: `corpus/manifest.toml` `# --- ramp backlog ---` (abseil → spdlog → eigen).

**Special case — Abseil (first run landed at E).** Unlike the format-heavy/template-front-door
libraries above, Abseil's value here is *quantity of complex, fully-specialized concrete data
types* — exactly what binds directly: each `absl::InlinedVector<T,N>`, `flat_hash_map<K,V>`,
`FixedArray<T>`, `int128` is a namable specialization the binder reflects head-on (no fixture
wrappers). The first run binds `InlinedVector<int,4>` + `<double,2>` to distinct Python types with
the full container surface, differentially checked vs native Abseil (incl. the small-buffer-spill
→ heap capacity growth). Genuine *special-case* facts established:
- Abseil is **not header-only** — even `InlinedVector::at()` references
  `absl::base_internal::ThrowStdOutOfRange` (in `throw_delegate.cc`). Added a reusable
  **`extra_sources`** path (`meta.toml` → `NB_EXTRA_SOURCES` → `build_module.sh` compiles+links the
  library `.cc`) — the minimal "link the library" route before a full prebuilt absl is warranted.
- Two real binder-coverage findings surfaced while scoping, **deferred** (type excluded, written
  up): **BINDER-0006** (`absl::FixedArray` internal zero-length `int[0]` storage member breaks
  `def_rw`) and **BINDER-0007** (`absl::int128`'s free `operator<<(std::ostream&,…)` makes the
  binder instantiate an incomplete `std::ostream` caster). Both have fix sketches.
- **Next Abseil step:** `flat_hash_map`/`flat_hash_set` (deeper link surface: `absl::hash` /
  `container_internal` `.cc` units) — likely a fuller prebuilt-absl link, and the binder fixes
  above unlock `int128`/`FixedArray`.

## Phasing (and where fan-out begins)

| Phase | What | Advance criteria | Status |
|---|---|---|---|
| **0** | manual single repo by hand (linalg), prove both CMake templates | one external lib at E + both templates proven | ✅ done |
| **1** | templatize machinery; run Tiers 0–2 human-in-loop | schema stable across ≥5 repos, aggregate renders, ≥1 A / ≥1 B / ≥2 E captured | ✅ **done** (fmt landed at E; Tiers 0–2 all banked) |
| **2** | **first subagent fan-out** — one dispatched agent does subset+tests on one repo, then 3–5 **serially** | ≥80% of agents produce schema-valid `result.json` unaided AND spot-checks confirm tests assert behavior (no import-only E masquerading) | 🟡 doorstep |
| **3** | **full fan-out** — long-tail manifest, tested concurrency pool, run by tier | continuous aggregation; A/B clusters fed back to the binder; `--rerun-failures` on each binder commit | ⬜ |

**Fan-out to subagents starts at Phase 2.** The `driver.py` orchestrator reads the
manifest and, per pending repo, materializes `runs/<slug>/` and **dispatches one
agent** scoped to that dir with the identical Gate 0–6 prompt (only `meta.toml`
differs, so results are comparable). Per-repo CWD + per-repo build dir = isolation.
Start serial → small pool (2–4) after confirming compile RAM/CPU contention is
acceptable (`template for` over real namespaces is heavy). Idempotent: a valid
`result.json` ⇒ done; after a binder change, bump `binder_commit` and
`--rerun-failures`. **Outcome diffs between binder commits are the campaign's real
product.**

## Toolchain-bug track (orthogonal to outcome)

Any A/B/C failure is triaged as `library` (genuinely unsupported), `toolchain`
(compiler/runtime bug), or `binder` (our codegen). Toolchain bugs are minimized to a
standalone `repro.cpp`, fingerprinted (so N repos hitting one ICE dedup to one
item), and fixed at the source via the `llvm-project` submodule (re-pinned).
`corpus/aggregate/toolchain_bugs.md` is the impact-ranked upstreaming queue.

## Status snapshot (as of Abseil reaching E)

8 corpus runs, **all E**: `_fixture_pod`, `_fixture_recursive`, `linalg`, `glm`,
`json`, `_fixture_virtual`, `fmt`, **`abseil`**. Fixes landed and pinned along the way:

- **BINDER-0001** templated members → skipped · **BINDER-0002** anonymous-union
  members → skipped · **BINDER-0003** spec-name enum NTTP (open, cosmetic; visible on
  fmt's `to_string<int>`→`to_stringInt0`) · **BINDER-0004** heavy-type perf de-dup ·
  **BINDER-0005** `concat()` ADL-hijack → fold rewrite + ADL-suppression · codegen
  trampoline dedup · `[[=reflect::skip]]` honored in transitive discovery.
- **BINDER-0006** (open, Abseil) `absl::FixedArray` internal zero-length `int[0]`
  storage member breaks `def_rw` → skip array-typed data members (fix sketched) ·
  **BINDER-0007** (open, Abseil) free `operator<<(std::ostream&,T)` bound as a dunder
  forces an incomplete `std::ostream` caster → skip stream operators in the
  free-operator scan (fix sketched).
- **TC-0001** Apple type-aware-allocation operators missing from from-source
  libc++abi → vendor shim compiled + exported (**fixed**).
- **TC-0002** clang Sema use-after-free under heavy reflection (the real cause of
  the apparent "raise-the-budget ICE"; originally misdiagnosed as an SLoc ceiling) →
  re-acquire the eval-context record across reentrant consteval (**fixed**).
- **LIB-0001** (new, fmt) fmt's `detail::allocator<T>` calls unqualified global
  `malloc`/`free` without `<cstdlib>`; strict two-phase lookup rejects it. Triage
  `library` (latent fmt bug, not toolchain/binder) → consumer-side `<cstdlib>`-first
  workaround, no fmt edit. fmt's all-template `format()`/`print()` front-ends are
  intrinsically not directly bindable (consteval `format_string<Args...>` first
  param); the real bindable formatting surface — enums, `format_int`, and
  `to_string<T>` instantiations — binds and is differentially checked.

Submodule pins carry these: `nanobind @ mk-reflect`, `llvm-project @
reflection-p2996`, `corpus/libs/json @ Cfretz244/json corpus-reflect-skip`,
`corpus/libs/fmt @ 11.2.0`, `corpus/libs/abseil @ 20250814.2`.

## Immediate next steps

1. **Abseil special case** — ✅ first run landed (E): `InlinedVector<int,4>`+`<double,2>` via the
   new `extra_sources` link path. **Next:** (a) land the **BINDER-0006/0007** fixes to unlock
   `FixedArray` and `int128` (the latter's operators are a rich dunder target); (b) take on
   `flat_hash_map`/`flat_hash_set`, which need the deeper `absl::hash`/`container_internal` link
   surface (likely a fuller prebuilt-absl static lib rather than per-`.cc` `extra_sources`).
2. **Enter Phase 2** — dispatch one subagent on `spdlog` (Tier 3: inheritance,
   shared_ptr, log-level enums) end-to-end; if it produces a schema-valid
   `result.json` unaided and its tests genuinely assert behavior, open a small serial
   batch. (Phase 1 is closed: fmt landed at E, Tiers 0–2 banked.)
3. **Phase 3** — expand the manifest to the long tail and raise concurrency, run by
   tier (easy first to bank wins), feeding A/B clusters back to the binder.

A pending follow-up (independent of the ramp): minimize **TC-0002** (the Sema UAF) to
a standalone reproducer with no nanobind/json, for an upstream bloomberg/clang-p2996
issue.
