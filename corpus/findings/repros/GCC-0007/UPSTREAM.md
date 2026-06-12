# GCC-0007 upstream record — constant-evaluation memory scales with total ops, not live data

- status: **REPORT READY** (performance/scalability report with root-cause
  analysis; no patch — the fix is architectural, see "Why no patch").
  Not yet filed — filing is the user's call.
- repros (this directory; all self-contained, no reflection involved):
  - `stress_churn.cpp` — the canonical case: consteval loop building
    transient heap-allocated `std::string`s (the emit generator's workload
    shape). Live data at any moment < 100 bytes.
  - `stress_newdelete.cpp` — pure `new int`/`delete` churn (isolates
    heap-var bookkeeping).
  - `stress_sso.cpp` — SSO-only string churn, no heap allocation at all
    (isolates non-heap evaluation garbage).
  - `stress_chunked.cpp` — same total work as stress_churn split into 20
    independent top-level constant evaluations (tests for collection
    points between evaluations).
- analysis tool: `alloc-profiling-instrumentation.patch` (apply to the GCC
  tree, rebuild cc1plus, dump histograms from gdb via
  `ggc_alloc_profile_dump()` / `tree_alloc_profile_dump()`).
- affected: GCC 16.1 (release) and trunk 17.0 @ 7ce3a7b1beb, identically.
  Field calibration (nlohmann/json binding-generator TU, identical source):
  clang-p2996 peaks at **1.69 GB**; g++ 16.1 is **OOM-killed above 31 GB**.

## Measurements (trunk @ 7ce3a7b1beb, aarch64-linux, container)

Peak cc1plus RSS, `-fsyntax-only -fconstexpr-ops-limit=1000000000`:

| repro | N=10000 | N=20000 | per-iteration retained |
|---|---|---|---|
| stress_churn | 1.20 GB / 72 s | 2.48 GB / 168 s | ~128 KB |
| stress_newdelete | 50 MB | 59 MB | ~0.9 KB |
| stress_sso | 173 MB | 248 MB | ~7.5 KB |
| stress_chunked (20 evals) | 1.21 GB / 71 s | — | ~128 KB (identical to one eval) |

stress_churn N=100000 exhausts a 31 GB machine. Wall time is superlinear
(2.3x for 2x N). clang's evaluator runs the equivalent workloads in a small
constant footprint.

## Root cause (instrumented + debugger-verified)

Every transient value the constexpr evaluator produces is allocated in GC
memory (`ggc_alloc` via make_node/copy_node/vec), and **none of it can be
collected while evaluation is in flight**: the evaluator and its callers
hold trees in C++ locals that are not GC roots, so there is no safe point
inside `cxx_eval_outermost_constant_expr`. Explicit `ggc_free` exists only
in narrow spots (parameter bindings at call teardown, the increment-expr
MODIFY_EXPR, `releasing_vec`s). Peak memory is therefore proportional to
**total operations**, not live data — measured **~8 bytes of garbage per
constexpr op** (32.6M ops at N=2000 produced 255 MB of GGC allocation).

Two gdb experiments on stress_churn N=2000 at the end of the evaluation
(breakpoint at the tail of `cxx_eval_outermost_constant_expr`):

1. **It is garbage, not table retention**: the evaluator tables are tiny
   (values map 4280 entries, call cache 2029, heap_vars 4000), but
   `G.allocated` = 254.9 MB. A forced `ggc_collect (GGC_COLLECT_FORCE)`
   from gdb drops it to **44.3 MB** — i.e. **83% of the retained memory is
   unreachable garbage** that simply has no collection opportunity.
2. **It is uniform, not one bad site** (per-tree-code histogram):
   `pointer_plus_expr` 1.46M nodes / 58.5 MB (every string-iterator step
   folds a fresh node over the heap var's address), `modify_expr` 326K,
   `array_ref` 284K, `component_ref` 188K, `constructor` 444K makes + 145K
   copies plus their elts vecs (the bulk of the remaining ~200 MB of
   uncleared allocation, from the `unshare_constructor`-on-init/store
   discipline). No single fixable allocation site dominates.

Why no collection point ever helps these workloads:

- The parser only collects after **function definitions**
  (`cgraph_node::finalize_function`, and `expand_or_defer_fn_1`'s
  template-junk collect in cp/semantics.cc — whose own comment notes that
  junk from template processing otherwise accumulates indefinitely).
- A namespace-scope sequence of static_asserts / constexpr variable
  initializers never passes one (stress_chunked: 20 independent top-level
  evaluations peak exactly like one big one).
- Real generator TUs are worse: a function-template body that names
  constexpr variable-template specializations evaluates ALL of them nested
  inside one `instantiate_pending_templates` cascade, where no parser-safe
  point can be interposed.

Why eager freeing cannot be extended much further (why this is
architectural): the evaluator freely returns **subtrees** of stored values
(e.g. `cxx_eval_component_reference` yields the CONSTRUCTOR element
directly), so a replaced or destroyed aggregate value may still be
referenced by another live value, the call cache, or the eventual result;
without ownership tracking, `free_constructor` on overwrite/death is a
use-after-free. (The existing parameter-binding free at call teardown is
guarded by exactly this: it frees only copies it made itself and checks
`c != result`.) The robust fix is an evaluator-local allocation arena (or
GC-rooted evaluation state) with results copied out — a design change, not
a spot fix.

## Bugzilla material (component c++, keywords memory-hog, compile-time-hog)

- Summary: `[C++26] constexpr evaluation memory scales with total operation
  count, not live data (~8 bytes/op never collected; consteval
  string-building OOMs at >31 GB where clang needs 1.7 GB)`
- Description: Measurements + Root cause sections above, with the table.
  Emphasize: no reflection needed — plain consteval `std::string` building;
  C++26 reflection/codegen workloads (P2996 emit generators) are where it
  bites at field scale, and `-fconstexpr-ops-limit` raises cannot help
  because memory, not the ops budget, is the binding constraint.
- Attach all four stress repros + the instrumentation patch (lets the
  maintainer reproduce both histograms in minutes).
- Cite as perf neighbors: PR 125179 (extremely slow build times with
  -freflection, fixed for 16.2 — wall-clock, does not move this memory
  behavior; we measure on trunk past it), PR 124925 (-freflection 20%
  slowdown). No existing memory-scaling report found (2026-06-11 sweep).
- CC: the PR120775 reflection series author; Patrick Palka, Marek Polacek,
  Jakub Jelinek.

## Relation to the corpus

This is what resource-walls the json / abseil_hash / abseil_btree gcc16
emit lanes (`[gcc16] emit_enabled = false` in their meta.toml; constexpr
lanes green, clang-p2996 emit lanes cover the gap). unordered_dense passes
at ~28.5 GiB peak (~91% of the container) — the margin case. Re-test all
four when any upstream movement happens on this report.
