# GCC-0005 upstream record — deferred dependent noexcept ICE (nothrow_spec_p)

- status: **RESOLVED UPSTREAM — no filing needed** (2026-06-12)
- repro: `gcc16-proveout/probes/xfail_gcc5_deferred_noexcept_partial_spec.cpp`
- upstream bug: **PR c++/124628** ("undeduced auto, deferred noexcept")

## Fixing commit (bisect-verified in the devenv, 2026-06-12)

- trunk: `05ea83ffd5409243902e1129e1e67ad7cff3afa6` (Patrick Palka,
  2026-05-14, "c++/reflection: undeduced auto, deferred noexcept
  [PR124628]"). Reflection queries now silently `mark_used` the function
  in an unevaluated context (like `requires { &decl; }`), which
  instantiates a deferred noexcept-specification (and does return-type
  deduction) before the type is handed out — so the binder's spliced
  `type_of(^^W<int>::swap)` no longer carries DEFERRED_NOEXCEPT into
  `most_specialized_partial_spec` → `nothrow_spec_p`.
- Verified empirically rather than by full bisect: probe compiles at
  `05ea83ffd54`, ICEs at `05ea83ffd54^` with the exact signature
  (`internal compiler error: in nothrow_spec_p, at cp/except.cc:1240`);
  log-scan of the 1505-commit candidate range surfaced no other
  noexcept-related C++ FE change.

## Backport status: ALREADY ON releases/gcc-16

- `e1396e44961be4856cdd91782d73b45014a4b276` (same author/subject,
  cherry-picked 2026-05-14, `releases/gcc-16.1.0-90-ge1396e44961`).
- ⇒ ships in **GCC 16.2**. Nothing to request.

## Consequences

- Do NOT file a new bug; PR 124628 covers it. (UPSTREAM_RESEARCH.md's
  earlier analysis that PR 113108's fix did not cover this path stands,
  but 124628 addressed the reflection-side trigger directly.)
- The binder's `nb_fn_type_of` workaround (force `is_noexcept(fn)` before
  `type_of(fn)`) stays until the container moves to 16.2; then re-run the
  probe smoke — `xfail_gcc5_*` should flip to passing and the shim can be
  retired (it remains behavior-neutral afterwards).
