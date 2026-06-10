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
| ★ | **special case — breadth of fully-specialized concrete data structures** | **Abseil** 20250814.2 — themed runs: `containers` (InlinedVector), `numeric` (int128/uint128), `time` (Duration/Time), `status` (Status/StatusCode), `crc` (crc32c_t), `statusor` (StatusOr<T>), `strings` (Cord + StrCat API), `civil_tz` (TimeZone/civil), `hash` (flat/node hash maps+sets), `btree` (btree_map/set) | ✅ E ×10 |

Concrete queue: `corpus/manifest.toml` `# --- ramp backlog ---` (abseil → spdlog → eigen).

**Special case — Abseil (four themed runs at E).** Abseil's value here is *quantity of complex,
fully-specialized concrete data types* bound head-on. Four themed sibling runs share one submodule
+ one prebuilt static lib:
- **`abseil_numeric`** — `absl::int128`/`uint128` bound directly: full arithmetic/bitwise/
  comparison **dunders** + working `str()`; differential on 128-bit results.
- **`abseil_time`** — `absl::Duration`/`Time` bound directly (operators → dunders, `operator<<` →
  `__str__`, e.g. `str(Seconds(90)) == "1m30s"`); a `timetest` fixture supplies the factory/
  conversion free functions (the types have no public integer ctors).
- **`abseil_status`** — `absl::Status` (`ok`/`code`/`raw_code`/`message`) + `StatusCode` enum,
  bound directly; exercises the `string_view`/`optional` casters.
- **`abseil_containers`** — `absl::InlinedVector<int,4>`/`<double,2>`, full container surface incl.
  the small-buffer-spill → heap-growth differential.

Infrastructure + fixes this established:
- Abseil is **not header-only** → a reusable **`link_abseil`** path: `lib/build_abseil.sh` builds
  absl at C++17 with the repo toolchain into one `libabsl_merged.a` (ABI-compatible with the C++26
  modules; shared libc++), and `NB_EXTRA_LIBS` links it (+ CoreFoundation for cctz) into both the
  module and the native oracle. The earlier per-`.cc` `extra_sources` path remains for tiny closures.
- Three binder fixes landed (re-pinned `nanobind @ ee245e6`): **BINDER-0006** array data members
  skipped; **BINDER-0007** free `operator<<(ostream&,T)` → `__str__` (genuine shifts still
  `__lshift__`); non-copy-assignable members → `def_ro`.

**The buildout (second Abseil campaign, June 2026).** Six more themed runs + four binder
features + four toolchain fixes, taking the corpus to **17 runs, all E**:
- **Easy wave:** `abseil_crc` (crc32c_t direct + crctest), `abseil_statusor` (StatusOr<int/
  string/double> specs; found the private-base using-re-export gap → BINDER-0009),
  `abseil_strings` (Cord head-on + strtest over the variadic StrCat/StrSplit API; found+fixed
  **BINDER-0010** by-value move-only params), `abseil_civil_tz` (TimeZone + nested CivilInfo +
  real `civil_time<tag>` internal-namespace specs).
- **Binder Change 1 — reachability-only discovery + base flattening:** a type binds only when
  reachable (seed or public-member signature); bases and a spec's own template args never
  qualify (no name-convention filters, per design decision). Unbound facade chains flatten
  onto the derived class; an in-set ancestor wires through unbound links as the Python base.
- **Binder Change 2 — member fn templates + entity proxies:** all-defaulted member templates
  bind via default instantiation (the heterogeneous-lookup shape); `using Base::f;`
  re-exports (incl. from PRIVATE bases — StatusOr's `value()`) bind through entity proxies
  (**requires `-fentity-proxy-reflection`**, not implied by `-freflection-latest`).
- **Containers head-on (BINDER-0008 closed):** `abseil_hash` (flat_hash_map/set,
  node_hash_map) and `abseil_btree` (btree_map/set) at E — full query surface
  (contains/count/find/erase/at/equal_range/lower_bound/`__getitem__`) bound directly, no
  policy-type explosion. Residuals: `FixedArray` (Abseil-internal `AsValueType` hard error —
  absl-side), container iteration (future `__iter__`/make_iterator feature), `insert()`'s
  `pair<iterator,bool>` return (no Python representation; population via fixtures).
- **Toolchain track:** **TC-0003** (entity-proxy metafunction ICEs + proxy-mangling
  defects — fixed locally and **upstreamed** as bloomberg/clang-p2996 #290 / PR #291;
  its qualifier-misreport addendum was minimized into **TC-0005** — lifetimebound
  AttributedType sugar, not proxies — fixed + upstreamed as #292 / PR #293) and
  **TC-0004** (**fixed locally** — was
  recorded as predicate misreports; minimization showed same-named function-template
  reflections as NTTPs mangled identically and codegen folded the dispatch instantiations.
  Mangler fix + standalone repro + regression tests landed; the binder's dispatch-level
  substitution workaround removed; upstreamed as #286 / PR #287).

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

## Status snapshot (as of the Abseil buildout)

**17 corpus runs, all E**: `_fixture_pod`, `_fixture_recursive`, `linalg`, `glm`,
`json`, `_fixture_virtual`, `fmt`, and TEN Abseil themed runs — `abseil_containers`,
`abseil_numeric`, `abseil_time`, `abseil_status`, **`abseil_crc`**, **`abseil_statusor`**,
**`abseil_strings`**, **`abseil_civil_tz`**, **`abseil_hash`**, **`abseil_btree`**.
Fixes landed and pinned along the way:

- **BINDER-0001** templated members → skipped · **BINDER-0002** anonymous-union
  members → skipped · **BINDER-0003** spec-name enum NTTP (open, cosmetic; visible on
  fmt's `to_string<int>`→`to_stringInt0`) · **BINDER-0004** heavy-type perf de-dup ·
  **BINDER-0005** `concat()` ADL-hijack → fold rewrite + ADL-suppression · codegen
  trampoline dedup · `[[=reflect::skip]]` honored in transitive discovery.
- **BINDER-0006** (**fixed**, Abseil) array data members (`T[N]`) skipped ·
  **BINDER-0007** (**fixed**, Abseil) free `operator<<(std::ostream&,T)` → Python
  `__str__` (genuine shifts still `__lshift__`) · non-copy-assignable members → `def_ro`
  (move-only `node_handle` no longer breaks `def_rw`). All re-pinned (`nanobind @ ee245e6`).
- **BINDER-0008** (**closed**, Abseil buildout) hash/btree containers bound head-on via
  reachability-only discovery + base flattening (Change 1) and default-instantiation
  member-fn-template binding (Change 2); `FixedArray` residual is Abseil-side
  (`AsValueType` zero-length array, retested) · also in the buildout: unary free
  operators → `__neg__`/`__pos__`/`__invert__` · widest integral conversion wins
  `__int__` · **BINDER-0009** (**fixed**) `using`-redeclarations (incl. from PRIVATE
  bases) bind via entity proxies · **BINDER-0010** (**fixed**) by-value move-only
  params skipped gracefully (Cord::Append(CordBuffer) was a TU-wide hard error).
- **TC-0001** Apple type-aware-allocation operators missing from from-source
  libc++abi → vendor shim compiled + exported (**fixed**).
- **TC-0002** clang Sema use-after-free under heavy reflection (the real cause of
  the apparent "raise-the-budget ICE"; originally misdiagnosed as an SLoc ceiling) →
  re-acquire the eval-context record across reentrant consteval (**fixed**; follow-up
  audit hardened `CheckLValueToRValueConversionOperand`, added a standalone repro +
  regression test, and upstreamed it as bloomberg/clang-p2996 #288 / PR #289).
- **TC-0003** (**fixed locally**, Abseil buildout; **upstreamed** as
  bloomberg/clang-p2996 #290 / PR #291) entity-proxy sharp edges: six metafunction ICEs
  (`is_constructor` et al.; the upstreaming pass probed the remaining unreachable arms
  and hardened `is_enumerable_type`/`has_complete_definition` too) + the Itanium-mangler
  defects on proxy reflections as NTTPs (root cause corrected during upstreaming:
  operator-named shadows crash `mangleUnqualifiedName`, and an overload set behind one
  using-declarator mangles all its shadow proxies identically — not the
  class-template-specialization shape originally blamed). Regression test added
  (`entity-proxy-member-queries.pass.cpp`); the `is_rvalue_reference_qualified`
  misreport through proxies was minimized into **TC-0005** (below) and fixed.
- **TC-0005** (**fixed locally**; **upstreamed** as bloomberg/clang-p2996 #292 / PR #293;
  promoted from TC-0003's addendum) function-type metafunctions were blind to
  AttributedType sugar: `[[clang::lifetimebound]]` methods (Abseil's
  `ABSL_ATTRIBUTE_LIFETIME_BOUND` accessors) silently reported all qualifiers false
  and their TYPE was rejected by `return_type_of`/`parameters_of` — proxies were
  incidental. Seven `dyn_cast<FunctionProtoType>` sites now desugar via `getAs`, as
  `is_noexcept` always did. Regression test
  (`attributed-function-type-queries.pass.cpp`); binder workaround comments rewritten
  (the `sizeof`-gate is the binder-matrix mechanism on every path, nothing to remove).
- **TC-0004** (**fixed locally**) originally recorded as `substitute()` predicate
  misreports under nested-dependent instantiation (silently dropped
  `raw_hash_map::operator[]`); the standalone repro showed the real mechanism —
  same-named function-template reflections as NTTPs mangled identically and codegen
  folded the sibling dispatch instantiations into one body. Mangler fix (ODR-hash
  discriminator) + repro + regression tests landed; binder workaround removed.
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

1. **Abseil buildout** — ✅ ten themed runs at E; BINDER-0008 closed (reachability
   discovery + member-fn-template + entity-proxy support landed); four toolchain fixes.
   **Residuals:** container iteration (`__iter__`/`make_iterator` binder feature — would
   also make `begin`/`lower_bound`/`Chunks()` surfaces usable), `FixedArray` (absl-side),
   a friendly-naming pass for spec-derived Python names (BINDER-0003 family;
   `flat_hash_mapIntStringHashInt`, `civil_timeDay_tag`).
2. **Enter Phase 2** — dispatch one subagent on `spdlog` (Tier 3: inheritance,
   shared_ptr, log-level enums) end-to-end; if it produces a schema-valid
   `result.json` unaided and its tests genuinely assert behavior, open a small serial
   batch. (Phase 1 is closed: fmt landed at E, Tiers 0–2 banked.)
3. **Phase 3** — expand the manifest to the long tail and raise concurrency, run by
   tier (easy first to bank wins), feeding A/B clusters back to the binder.

Pending follow-ups (independent of the ramp): **TC-0002 through TC-0005 are upstreamed**
(bloomberg/clang-p2996 #286/PR #287, #288/PR #289, #290/PR #291, #292/PR #293 — track to
merge). TC-0003's addendum is CLOSED: minimization promoted it to **TC-0005** (the
qualifier misreport was never proxy-specific — `[[clang::lifetimebound]]`'s
AttributedType sugar blinded a family of `dyn_cast<FunctionProtoType>` sites; fixed +
filed). The binder's `sizeof`-gate turned out to be its universal binder-matrix
mechanism, not a bespoke workaround — its stale rationale comments were rewritten.
