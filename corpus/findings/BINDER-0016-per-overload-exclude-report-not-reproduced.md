# BINDER-0016 — wave-1 report: per-overload `nb::exclude_` ineffective (NOT REPRODUCED)

- **Status:** INVESTIGATED — not reproduced; one defensive hardening landed
  (`is_exclude_marker` now dealiases, so a marker reached through a `using` alias is
  recognized instead of silently excluding nothing).
- **Reported via:** corpus/runs/tomlplusplus (wave 1; dedup key
  `binder-member-overload-exclude-ineffective`). The agent collected the two
  `is_homogeneous(node_type, node*&)` overload reflections via `members_of`, built a marker with
  `substitute(^^nb::exclude_, {reflect_constant(info)...})`, verified `==` round-trips 2/2, and
  reported the overloads STILL bound (hard error at `nb_func.h:292`).
- **Investigation (driver):** two standalone repros against the exact suspected mechanisms, both
  PASS on the pinned toolchain:
  1. `reflect_constant`/`substitute`/`template_arguments_of`/`extract<info>` round-trip preserves
     identity for same-named member overloads (incl. virtual + noexcept + const/non-const pairs).
  2. The binder-shaped gate — `excluded_v` as a `define_static_array` of extracted marker args,
     consulted via `info_span_contains` against a *separately lifted* `members_of` walk, with
     out-of-line definitions adding redeclarations — correctly excludes both overloads and leaves
     siblings bound. (Repros preserved under `corpus/findings/repros/BINDER-0016/`.)
  The existing suite also covers the literal-NTTP marker path (`test41`, `xvec_member("doomed")`).
- **Assessment:** the failure was most likely in the run's marker plumbing (the agent's exact TU
  state is not recoverable; its final binding excludes the whole node tree instead). The
  triggering shape is moot under BINDER-0015 — `T*&` methods now skip automatically. If a wave-2+
  agent reproduces per-overload exclusion failure with a concrete TU, reopen with that TU pinned.
