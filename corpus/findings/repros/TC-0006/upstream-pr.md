Fixes #294.

## Problem

`can_substitute` — the SFINAE probe that is supposed to answer `false` for failing substitutions — crashed the frontend when substituting the (individually valid) arguments into the *declaration* formed an invalid type inside a template-id, e.g. a reference to void: the classic `enable_if`-era immediate-context failure (see #294 for self-contained reproducers and crash signatures). `CheckTemplateArgumentList` succeeds (void is a fine argument for `class OT`), then per template kind: function templates **SIGSEGV** (`InstantiateFunctionDeclaration` runs in a SFINAE context and returns null; `MetaActionsImpl::Substitute` dereferenced it), alias templates **leak a hard error** out of `CheckTemplateIdType` and go non-constant, variable templates **trip the post-`Substitute` assert** ("substitution failed after validating arguments?"). The field shape is `tl::expected<void, E>`'s `swap` member template, whose defaulted parameter picks up the enclosing specialization's `void` argument — merely probing members with `can_substitute({})` crashed the compiler.

## Fix

- **`clang/include/clang/AST/MetaActions.h`** — the four `Substitute` overloads gain a `bool SuppressDiagnostics` parameter (matching the existing `CheckTemplateArgumentList` contract) and a documented null-on-failure contract.
- **`clang/lib/Sema/SemaReflect.cpp`** — each override wraps its Sema call in `Sema::SuppressDiagnosticsRAII` when requested; the `FunctionTemplateDecl` override null-checks `InstantiateFunctionDeclaration`'s result before the `Spec->getType()` dereference (the SEGV); the alias/var/concept overrides already produce null on failure and are now diagnostically clean.
- **`clang/lib/AST/ExprConstantMeta.cpp`** — `substitute` replaces the post-`Substitute` asserts / ignored failures with the `NoDiagnose` pattern for all four kinds: `can_substitute` → `false` (null reflection via `ElideDiagnosis`), `substitute` → non-constant with the new `metafn_substitution_failed` note — the same shape as the existing `metafn_undeduced_placeholder` path.
- **`clang/include/clang/Basic/DiagnosticMetafnKinds.td`** — the new note.

## Test

- `libcxx/test/std/experimental/reflection/can-substitute-invalid-type-formation.pass.cpp` (new): the free-function-template `{^^void}` shape, the defaulted-parameter member template probed through a `members_of` walk (`Exp<void>` vs the `Exp<int>` control), and the alias/variable-template siblings — each previously a crash/leak/assert, now reporting failure; positive controls assert non-void substitutions still succeed.
- `substitute.verify.cpp` (extended): pins the diagnosing behavior — `substitute(^^fn, {^^void})` is non-constant with the note `substitution of the given template arguments into 'fn' failed`.

## Validation

- Base: `837da39eb88c` (current `p2996` tip; applies cleanly, builds standalone).
- Without the fix: the reproducers in #294 SIGSEGV (function), hard-error (alias), assert (variable); the new test crashes the compiler.
- With the fix: reproducers and the new tests compile and run clean.
- `clang/test/Reflection`: 16/16 with the fix on this machine — identical to base.
- Downstream soak (reflection-driven nanobind binding generator): 53-test binder suite green; real-library corpus runs including TartanLlama/expected v1.3.1 with `tl::expected<void, std::string>` IN the bind set — the binder's `can_substitute({})` probes on `swap<OT=void>`/`value<U=void>` now report not-substitutable and the members are gracefully skipped, with the full differential suite passing.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
