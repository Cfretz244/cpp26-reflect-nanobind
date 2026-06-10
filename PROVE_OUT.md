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
| 3 | inheritance; shared_ptr; log-level enums | **spdlog** v1.17.0; **tl::expected** v1.3.1 | ✅ E |
| 4 | virtual override → two-stage codegen | `_fixture_virtual` | ✅ E |
| 5 | stress ceiling; expression templates | **Eigen** (`Matrix<double,3,3>`); a header-only Boost piece | ⬜ frontier |
| ★ | **special case — breadth of fully-specialized concrete data structures** | **Abseil** 20250814.2 — themed runs: `containers` (InlinedVector), `numeric` (int128/uint128), `time` (Duration/Time), `status` (Status/StatusCode), `crc` (crc32c_t), `statusor` (StatusOr<T>), `strings` (Cord + StrCat API), `civil_tz` (TimeZone/civil), `hash` (flat/node hash maps+sets), `btree` (btree_map/set) | ✅ E ×10 |

Concrete queue: `corpus/manifest.toml` `# --- ramp backlog ---` (tl::expected → eigen).

**Tier 3 — spdlog (June 2026).** spdlog v1.17.0 at E, handled directly (libraries are not
yet "flying by"; Phase 2's first unaided subagent run is still pending). `spdlog::logger`
bound head-on (ctors incl. `(string, sink_ptr)`, the non-template `log(level, string_view)`
overloads, set_level/should_log/set_pattern/flush/sinks()/clone), `spdlog::level` namespace
(enum + `to_string_view`/`from_str`), and the sink hierarchy as REAL Python bases:
`stdout_sink_mt` → `stdout_sink_base<console_mutex>` → abstract `sink`. A `logtest`
CaptureSink fixture (ostringstream-backed `ostream_sink` handed out as `shared_ptr<sink>`)
makes logger output observable from Python; the L1 differential compares formatted output
byte-for-byte against the native oracle driving the identical scenario (pattern, level
filtering, clone-shares-sinks). Built under `SPDLOG_USE_STD_FORMAT` (a first-class spdlog
config): default bundled-fmt mode's `string_view_t` = `fmt::basic_string_view`, which no
caster covers. Found+fixed **BINDER-0011**: (1) abstract classes got `nb::init` bound (TU
hard error; now no ctors unless a trampoline is registered — then init constructs the
Alias), and (2) the STL-caster matrix collected signatures of methods the binder never
binds (`[[=reflect::skip]]` / BINDER-0010 move-only-by-value, e.g.
`sink::set_formatter(unique_ptr<formatter>)`) — the walk now mirrors the bind-path skip
predicates.

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

## Status snapshot (as of the spdlog run)

**18 corpus runs, all E**: `_fixture_pod`, `_fixture_recursive`, `linalg`, `glm`,
`json`, `_fixture_virtual`, `fmt`, **`spdlog`** (Tier 3 banked), and TEN Abseil themed
runs — `abseil_containers`, `abseil_numeric`, `abseil_time`, `abseil_status`,
`abseil_crc`, `abseil_statusor`, `abseil_strings`, `abseil_civil_tz`, `abseil_hash`,
`abseil_btree`.
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
- **BINDER-0011** (**fixed**, spdlog) abstract classes bound `nb::init` (any abstract
  interface in the bind set was a TU-wide hard error) → ctor pass now skipped for
  abstract classes WITHOUT a registered trampoline (with one, init constructs the Alias
  — the Python-subclass path, codegen tests cover it); plus the STL-caster matrix
  over-collected: signatures of never-bound methods (skip-annotated / BINDER-0010
  move-only-by-value) no longer demand casters (`nanobind @ beda45a`).
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
`corpus/libs/fmt @ 11.2.0`, `corpus/libs/abseil @ 20250814.2`,
`corpus/libs/spdlog @ v1.17.0`.

## Immediate next steps

1. **Abseil buildout** — ✅ ten themed runs at E; BINDER-0008 closed (reachability
   discovery + member-fn-template + entity-proxy support landed); four toolchain fixes.
   **Residuals:** container iteration (`__iter__`/`make_iterator` binder feature — would
   also make `begin`/`lower_bound`/`Chunks()` surfaces usable), `FixedArray` (absl-side),
   a friendly-naming pass for spec-derived Python names (BINDER-0003 family;
   `flat_hash_mapIntStringHashInt`, `civil_timeDay_tag`).
2. **spdlog (Tier 3)** — ✅ E, handled directly (one more direct run before fan-out, per
   the not-yet-flying-by call). BINDER-0011 found+fixed. **Residual:** Python-side custom
   sinks (sink's pure virtuals via a two-stage trampoline) deliberately out of scope.
3. **Phase 2 — first unaided subagent run: ✅ dispatched and PASSED its bar.**
   `tl::expected` v1.3.1 ran end-to-end under the identical-prompt protocol
   (`corpus/lib/AGENT_PROMPT.md`, written for this dispatch and reused verbatim for
   every later repo). The agent produced a schema-valid `result.json` unaided, held
   every guardrail (no commits, no binder/toolchain edits), bound the real API
   (`expected<int,string>` + role-swapped spec, `unexpected`, `bad_expected_access`;
   fixtures only for template-ctor factories), wrote 10 genuinely-differential tests
   (latent, committed), and — the payoff — recorded outcome **B** with a four-bug
   cascade, each with a verified minimized repro: **TC-0008** (deduction-guide
   reflection ICEs the Itanium mangler — `unexpected(E) -> unexpected<E>` at namespace
   scope means binding ANY tl class dies at codegen; TC-0004's component family),
   **BINDER-0012** (ctor pass lacks an `is_deleted` filter), **TC-0006**
   (`can_substitute` SIGSEGVs forming `void&` in a template-id instead of returning
   false), **TC-0007** (`members_of` wrong-instantiates member bodies when it is what
   first instantiates the spec). Driver-verified: repros reproduce, controls clean.
   **The cascade is FIXED and the run is at ✅ E** (16/16 differential tests, the void
   spec back in the pack). Fixing it surfaced two more: **TC-0009** (same-headed
   member-template reflections of a specialization mangled identically as NTTPs —
   `ODRHash::AddFunctionDecl` no-ops in specialization context; `value()` silently
   never bound, caught by the run's differential suite on its first post-fix
   execution — a gap in TC-0004's fix) and **BINDER-0013** (copy ctors were never
   bound; `init<const T&>` now binds for copyable non-trampolined classes).
   **Next: open the small serial batch.**
4. **Enter Phase 2 batch** — after the expected blockers land, dispatch 3–5 repos
   serially under the same prompt. (Phase 1 is closed: fmt landed at E, Tiers 0–2
   banked.)
5. **Phase 3** — expand the manifest to the long tail and raise concurrency, run by
   tier (easy first to bank wins), feeding A/B clusters back to the binder.
6. **Phase 3, wave 1: ✅ COMPLETE — first PARALLEL fan-out, 8 Opus agents at once,
   8/8 at E after triage.** The campaign plan: 24 new libraries over 3 waves of 8
   (graded easy→frontier; wave 3 carries the compiled heavyweights re2/JoltPhysics/
   Botan), driver checkpoint per wave. Parallel-dispatch protocol added to
   `AGENT_PROMPT.md` (driver-side Gate 0 — concurrent `git submodule add` races the
   index; per-run `findings_draft/` with `dedup_key`, canonical numbers assigned at
   triage; per-run pytest cache). Wave 1 = tomlplusplus, cli11, date,
   unordered_dense, pugixml, tinyobjloader, fast_float, concurrentqueue. **All 8
   agents finished unaided** (8/8 schema-valid, >= the 80% bar) at 6 E / 1 D / 1 B;
   an independent spot-check audit verified every E differential genuine. Triage
   clustered 8 draft findings into **BINDER-0015..0021** (zero toolchain bugs from
   the wave itself — the agents' findings were all binder-layer): 0015 unbindable
   shapes (ptr-to-ptr / `T*&` out-params / fn-pointers) now skip gracefully
   everywhere; 0017 raw class-pointer returns default to BORROWING policies (the
   cli11 D-blocker: take_ownership double-freed CLI11's fluent API); 0018
   `typedef struct {...} name_t;` binds under the typedef name threaded from the
   walk (the tinyobjloader B-blocker); 0019 plain-class completeness gates
   (pugixml's pImpl); 0020 const statics bind by value (no ODR-use link errors —
   moodycamel's config constants); 0016 per-overload exclude_ report NOT
   reproduced (repros banked; `is_exclude_marker` dealias hardening); 0021 open
   design note (uncastable-member property). Implementing 0020's probe surfaced
   **TC-0013** (dependent splice as `auto`-NTTP argument in a requires-expression
   ICEs at parse — driver-found, fixed, regression-tested, 16/16
   `clang/test/Reflection`). Harness classifier false-positive fixed
   (`static assertion failed` no longer tags compiler-crash). Re-gates: binder
   suite 58/58 (4 new tests); cli11 D→E, tinyobjloader B→E; full-corpus sweep
   **28/28 E** — which caught a real regression (json's two-stage generator blew
   its 100M constexpr-step budget under the new per-member gates → 400M, green),
   then a second full sweep on the TC-0013-fixed compiler. **Next: wave 2**
   (yaml-cpp, simdjson, cpp-httplib, leveldb, SQLiteCpp, taskflow, Box2D 2.4.x,
   immer).
7. **Phase 3, wave 2: ✅ COMPLETE — the compiled/stateful tier, 8/8 at E after
   triage; the richest toolchain-bug wave of the campaign (FOUR new TCs).**
   Harness grew the generic prebuilt-archive path (`build_cmake_lib.sh` + the
   `extra_libs` meta key, `{repo}`-portable) — yamlcpp/leveldb/sqlitecpp/box2d
   link driver-prebuilt merged archives; sqlitecpp bundles its sqlite3. All 8
   agents finished unaided (7 E / 1 B raw); the independent audit verified
   every E genuine (fixture-heavier tier by nature — servers on threads, DB
   out-param fronts, task-graph callables — but all thin forwarding/lifetime
   anchors over real bound types). Findings: **BINDER-0022..0028** (enum/class
   module-name collisions bind parent-qualified — yamlcpp's NodeType::value
   clobber; cv-void* skip; same-name member+static import ABORT skipped;
   T& returns borrow like T* — taskflow's policy=copy abort; reflected ctors
   construct with PARENS — immer's initializer_list hijack; namespace aliases
   not followed — simdjson's fixture alias bound the world; std::function-arg
   exclusion recursion recorded OPEN), **LIB-0003** (leveldb -fno-rtti:
   typeinfo absent, head-on DB binding impossible — will recur for Google
   libs) and **LIB-0004** (taskflow v4 missing <bit>). Toolchain: **TC-0014**
   (DescriptionOf ICE'd on builtin templates — the box2d B-blocker: EVERY
   global-namespace class died; with the fix the same TU compiles clean
   because the error was always handled, only its MESSAGE crashed),
   **TC-0015** (deduction-guide SPECIALIZATION reflections ICE'd the mangler —
   the Declaration-kind sibling of TC-0008, exposed by the simdjson alias
   leak), **TC-0016** (members_of silently TRUNCATED at `extern "C" { typedef
   struct X X; }` — Python.h's pytypedefs.h hid every later global decl from
   reflection in every nanobind TU; box2d's free operators "didn't exist";
   fix walks the lexical block + de-cycles the namespace MultDC tail-hop),
   and **TC-0017** (64-bit NEON vectors with LP64 `long` elements ICE'd the
   mangler — driver-found writing TC-0016's regression test; likely
   reproducible in upstream clang). box2d re-gated B→E with byte-identical
   90-step physics; full-corpus sweep green on the final compiler. Upstream
   batch: four issues/PRs (TC-0016 stacked on TC-0011's PR), all on #308.

Pending follow-ups (independent of the ramp): **TC-0010..0012 are upstreamed**
(#302/PR #305, #303/PR #306, #304/PR #307); **TC-0013** (wave 1, driver-found) is
fixed locally with its upstream filing in flight — all tracked on the campaign
issue bloomberg/clang-p2996#308. **TC-0002 through TC-0009 are upstreamed**
(bloomberg/clang-p2996 #286/PR #287, #288/PR #289, #290/PR #291, #292/PR #293, and the
expected-run batch #294/PR #295 (TC-0006), #296/PR #297 (TC-0007), #298/PR #299
(TC-0008, stacked on #287), #300/PR #301 (TC-0009, stacked on #299) — track to merge;
per-finding `repros/TC-000N/UPSTREAM.md` has each filing's evidence). TC-0003's addendum is CLOSED: minimization promoted it to **TC-0005** (the
qualifier misreport was never proxy-specific — `[[clang::lifetimebound]]`'s
AttributedType sugar blinded a family of `dyn_cast<FunctionProtoType>` sites; fixed +
filed). The binder's `sizeof`-gate turned out to be its universal binder-matrix
mechanism, not a bespoke workaround — its stale rationale comments were rewritten.
