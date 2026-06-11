# FILED — bloomberg/clang-p2996 issue + PR (2026-06-11)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/319
- **PR:** https://github.com/bloomberg/clang-p2996/pull/320 — branch
  `reflect-symbol-of-caret-equals` on `Cfretz244/llvm-project` (commit `a1a87f7`:
  `ff46ef7` cherry-picked onto their `p2996` tip `837da39`; the commit message
  was already free of internal finding tags; code comments clean).
- **Tracking:** listed on https://github.com/bloomberg/clang-p2996/issues/308
  under the new category **F. std::meta library data** (tick when merged).

## Validation evidence behind the filing (all on this laptop)

- Header-only fix (libcxx `<meta>` data tables), so PR-state validation used
  the toolchain clang with the PR branch's `meta`/`experimental/meta` headers
  overlaid first on the include path (`-isystem /tmp/tc19-shim`):
  - `repro.cpp` (both static_asserts) compiles clean on the PR state;
  - the regression test `operator-symbol-tables.pass.cpp` compiles AND runs
    (exit 0) on the PR state;
  - the repro FAILS against the pristine-base headers (`git show
    837da39:libcxx/include/meta` overlaid the same way): 2 static assertion
    failures — both-direction validation.
- Full local toolchain (reflection-p2996 with the same fix): binder suite
  131/131, full 36-run corpus three-way E (the abseil_numeric run is the one
  that found it: its emit lane went B.emit_compile -> E with this fix).

## Found-via provenance

The emit backend (production-toolchain source codegen) renders operator
bindings as explicit `self.operator^=(...)` calls; `symbol_of` supplied the
spelling. The constexpr backend splices `[:fn:]` and never consults the
table — which is why the bug survived every earlier corpus wave and only
surfaced when a textual renderer consumed the value.
