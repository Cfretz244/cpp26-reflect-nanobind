# TC-0002 — clang Sema use-after-free under heavy reflection (FIXED)

- **Status:** root-caused + **fixed in the pinned toolchain**
- **Kind:** use-after-free (memory-safety bug in clang Sema)
- **Toolchain:** `llvm-project` clang-p2996, `clang/lib/Sema/SemaExpr.cpp`
- **Found via:** Phase 1, nlohmann/json — `nb::reflect_<^^nlohmann::json>` over `basic_json`.

## CORRECTION of the original finding

The first version of this finding claimed `basic_json` was **"intractable on this toolchain"**:
that reflecting it exceeded the constexpr step budget, and that raising `-fconstexpr-steps`
ICE'd the compiler via **SLoc address-space exhaustion**. **All of that framing was wrong.**
Evidence that disproved it:

- The crash under a raised step budget is a **non-deterministic SIGSEGV** (varying *ASCII-looking
  garbage* fault addresses across runs: `0x4c453734634c4538`, `0x634c453734634c4d`, `0x74`), at a
  constant instruction in `Sema::PopExpressionEvaluationContext`. Varying garbage = heap
  corruption / use-after-free, **not** deterministic address-space exhaustion.
- **Peak memory ~200 MB** — nowhere near any limit; not memory or SLoc exhaustion.
- Shallow 17-frame stack — not a stack overflow.

The "Invalid SLocOffset" assert seen earlier was a *downstream symptom* of the same corruption,
not the cause.

## Real root cause (use-after-free)

`Sema::PopExpressionEvaluationContext` holds `Rec = ExprEvalContexts.back()` — a **reference into
the `SmallVector<ExpressionEvaluationContextRecord, 8> ExprEvalContexts`**. It then calls
`HandleImmediateInvocations(*this, Rec)`, which evaluates pending consteval immediate invocations
(here, `emit_trampolines<^^nlohmann::json>` / the `reflect_` machinery). That evaluation does deep
nested **template instantiation**, which recursively **pushes (and pops) expression-evaluation
contexts**. Once `ExprEvalContexts` grows past its inline capacity it **reallocates**, leaving
`Rec` (and the post-loop uses in `HandleImmediateInvocations`, plus the tail of
`PopExpressionEvaluationContext`: `Rec.VolatileAssignmentLHSs`, `Rec.SavedMaybeODRUseExprs`, …)
**dangling into freed memory** — which gets reused for `define_static_string` text, hence the
ASCII-garbage pointers.

The fork already half-knew this — there is a `NOTE(P2996)` and an index-based loop guarding the
*candidate vector* from growth — but it missed the outer `Rec`-reference invalidation from the
`ExprEvalContexts` reallocation itself.

**Why raising `-fconstexpr-steps` "ICE'd" the compiler:** the default step budget hit the
step-limit diagnostic *before* the corruption could surface. Raising it just let evaluation run
far enough to dereference the dangling `Rec` → crash. The step budget was a red herring; the
budget was never the wall.

## Fix

`clang/lib/Sema/SemaExpr.cpp`: re-acquire the record (always the top of the stack) after any
reentrant evaluation, rather than holding the reference across it —
`HandleImmediateInvocations` re-fetches via `SemaRef.currentEvaluationContext()` after the
candidate loop; `PopExpressionEvaluationContext` re-fetches `ExprEvalContexts.back()` for its tail
(and around `CleanupVarDeclMarking()`).

## Validation

- `corpus/runs/json/binding/gen.cpp` and the full `^^nlohmann::json` module now compile (≈17 s,
  `-fconstexpr-steps≈20M` — a legitimate budget for a type this large, no crash).
- Regression-clean: nanobind reflection suite 41/41; `clang/test/Reflection` 16/16; a
  consteval/immediate-invocation lit sweep 18/0.
- nlohmann/json reaches **outcome E** (real class bound, L1 differential vs a native oracle).

dedup_key: `sema-eval-context-uaf-reentrant-consteval`. Upstreamable to bloomberg/clang-p2996
(then mainline LLVM) — confirmation reproducer: any reflection that evaluates a large immediate
invocation pushing >8 nested eval contexts.
