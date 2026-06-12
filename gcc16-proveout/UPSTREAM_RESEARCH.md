# Upstream research: do our GCC findings already exist on gcc.gnu.org?

Researched 2026-06-11 against GCC bugzilla (via the gcc-bugs list mirrors —
bugzilla itself blocks automated fetches; verify bug states by hand before
filing). Context: GCC 16.1's reflection landed as one ~51k-line series under
the tracking bug **PR c++/120775** ("[C++26] P2996R13 - Reflection"); its
author acknowledged ~50 in-code TODOs/FIXMEs at merge. There is a busy
stream of reflection bugs being filed and fixed for 16.2, so RE-CHECK
against trunk/16.2 snapshots before filing anything below.

## Per-finding status

| ours | upstream match | action |
|---|---|---|
| GCC-1 (lift instantiates implicit special member defs) | none found | file (group with GCC-6, likely same root) |
| GCC-2 (constexpr local rejected as expansion range in template) | closest: **PR 123379** (template for fails with temporary std::span, Jan 2026) — related range-usability handling, not our shape | file, cite 123379 |
| GCC-3 (lambda splicing enclosing NTTP is consteval-only, can't decay) | none found | file as a QUESTION (may be conforming under consteval-only propagation; ask) |
| GCC-4 (discarded if-constexpr branches in expanded bodies checked) | closest: **PR 123611** (bogus "consteval-only expressions only in constant-evaluated context" in expansion statements, FIXED Feb 2026 for 16.x) — adjacent, not ours | file as divergence question, cite 123611 |
| GCC-5 (nothrow_spec_p ICE, deferred dependent noexcept) | **PR 113108** is the same assert via OVERLOAD RESOLUTION — RESOLVED/FIXED in r15-3455 (GCC 15) + r14-11168. A fresh nanobind-related dup (**PR 125630**, "ICE in nothrow_spec_p with nanobind overload_cast on CGAL", June 2026) was duped to it. We reproduce on 16.1, which contains that fix ⇒ our trigger (most_specialized_partial_spec matching a reflection-spliced function type; also is_noexcept on the TYPE) is a DISTINCT unfixed path | file NEW bug, cite 113108 + 125630, note the fix does not cover this path |
| GCC-6 (lift instantiates constexpr member BODY; lazily-ill-formed body hard-errors) | none found | file (with GCC-1; xfail_gcc6 probe is the repro) |
| GCC-7 (constant-evaluation memory blow-up rendering consteval strings; cc1plus OOM >31 GB where clang needs 1.7 GB) | no memory bug found. Perf neighbors: **PR 125179** ("extremely slow build times with -freflection", 40x, FIXED by P. Palka for **16.2**) and **PR 124925** (-freflection 20% slowdown). 125179's fix may or may not move memory | RE-TEST on 16.2 once released; if still >15x, file as performance/memory report citing 125179 |
| GCC-8 (same-named member-template reflection NTTPs mangle identically → "symbol already defined") | none found. Mangler neighbor: **PR 123237** (ICE in write_type mangling dependent splices, FIXED r16-8592 for 16.2 by M. Polacek) — different shape (ICE on dependent splice, not silent name-only mangling of Template-kind reflections) | file NEW bug, cite 123237 as the area; emphasize the silent-collision variant (linkonce folding) is the dangerous mode |

## Other context picked up

- The reflection-bug stream is active (June 2026): PR 125759 (ICE reflecting
  a reference pack), 125753 (-g + splice alias in expansion statement),
  125570 (ICE splicing member function template w/o explicit args), 125601
  (`break` in `template for` escapes to enclosing loop during constant
  evaluation — could bite consteval walks!), 125280 (incomplete-type errors
  under -freflection), 125206 (ICE in bases_of), 123611/123379 (expansion
  statements). Worth a skim before filing to avoid dups and to flag
  anything that could bite the binder.
- Modules + reflection is broken in several ways (PR 124709/124919/122785)
  — irrelevant to the corpus today, relevant if the binder ever modularizes.
- GCC 16.2 will carry many reflection fixes (125179, 123237, 124646,
  124792...): when the gcc:16 docker tag moves to 16.2, re-run the
  gcc16-proveout/probes/ conformance smoke (0*.cpp must pass, xfail_* may
  start passing — recheck each) and re-test GCC-0007.

## Trunk verification matrix (2026-06-12, master = 17.0.0 @ 7ce3a7b1beb)

Every probe re-run against a fresh trunk build (devenv/, `--enable-checking`):

| probe | trunk result | filing consequence |
|---|---|---|
| xfail_gcc5 (deferred noexcept) | **FIXED on trunk** | do NOT file a new bug; identify the fixing commit (bisect in devenv) and request/verify a releases/gcc-16 backport instead |
| xfail_gcc1 (implicit-member lift) | still fails (now surfaces via stl_construct.h overload error) | file |
| xfail_gcc2 (constexpr local range) | still fails ("'indices' is not a constant expression") | file |
| xfail_gcc3 (consteval lambda decay) | still fails | file as question |
| xfail_gcc6 (constexpr body on lift) | still fails (same m_val error) | file |
| xfail_gcc8 (NTTP mangling collision) | still fails — and trunk's checking build upgrades it to "Two symbols with same comdat_group are not linked by the same_comdat_group list" (symtab verify) | file with BOTH signatures (16.1 assembler error + trunk checking ICE — the latter proves it at the compiler layer) |
| 09_discarded_splice (GCC-4 family) | still fails, NEW shape: "uncaught std::meta::exception: reflection does not represent a type" | file as question with both behaviors |
| positive probes 01–07, 09b | all pass on trunk | — |
| 08_ann_spec | breaks on trunk — NOT a regression: GCC 17 removed the transitional two-arg `annotations_of(r, type)` overload (P2996R13 final spells it `annotations_of_with_type`, which the binder's shim already uses) and `constexpr auto` of a `std::vector` result is correctly rejected as non-transient allocation | update the probe when the corpus moves past 16.x; no filing |

## FINAL DISPOSITION (2026-06-12 — minimize/root-cause/fix pass complete)

Every finding worked to a terminal state in the devenv (fix commits live on
the devenv checkout's `proveout-fixes` branch, base master `7ce3a7b1beb`;
patches + per-finding UPSTREAM.md with ready-to-file material in
`corpus/findings/repros/GCC-000N/`). Nothing has been filed — filing is the
user's call.

| ours | disposition |
|---|---|
| GCC-1 + GCC-6 | **PATCH READY** (one fix, `a2b10c8601f`): the P0859 pre-pass walked into REFLECT_EXPR operands, instantiating/synthesizing every reflected member on a lift. Both probes flip to passing; testsuite slices 3939/0. File as ONE bug. |
| GCC-2 | **RECLASSIFIED, conforming — do not file**: [stmt.expand]/5.2 binds a constexpr REFERENCE to the parenthesized range; a non-static constexpr local has no constant address. GCC's fix-it (`static`) verified. clang accepts-invalid. |
| GCC-3 | **PATCH READY** (`400daa86161`): splice operands are manifestly constant-evaluated and must not consteval-escalate the enclosing lambda. Probe flips; regression test added. |
| GCC-4 / probe 09 | **RECLASSIFIED, conforming — do not file**: [meta.reflection.traits] predicates THROW for non-type reflections (clang's false-return is the divergence); the "discarded branch checking" was cascade from the failed classify — an is_type-gated hybrid passes on 16.1 and trunk. |
| GCC-5 | **RESOLVED UPSTREAM — do not file**: fixed by trunk `05ea83ffd54` (PR c++/124628, Palka), bisect-verified; already on releases/gcc-16 as `e1396e44961` (16.1.0-90) ⇒ in 16.2. Retire the binder's `nb_fn_type_of` shim at 16.2. |
| GCC-7 | **REPORT READY, no patch (architectural)**: constexpr evaluation retains ~8 bytes/op of uncollectable GC garbage (83% proven unreachable via forced collect; per-tree-code histogram uniform); no collection point is reachable from constexpr-heavy declaration/instantiation contexts. File as performance/memory report with the four stress repros + instrumentation patch. |
| GCC-8 | **PATCH READY** (`be4033289d7`): Template-kind reflection NTTPs mangled by scope+name only; function templates overload. `ft` encoding extended with template head + pattern function type + constraints. Probe flips; mangle8.C added; 3939/0. The strongest filing (wrong-code/silent comdat fold). |

## Filing notes (Phase 4)

- File on gcc.gnu.org/bugzilla, component **c++**, with `-freflection` in
  the keywords/summary; CC the reflection implementers (the PR120775
  series author and the active fixers seen above: Patrick Palka, Marek
  Polacek, Jakub Jelinek).
- Each filing mirrors the TC-XXXX discipline: minimized probe from
  gcc16-proveout/probes/ + expected-vs-actual + clang-p2996 cross-check;
  record the filed number in the corresponding corpus/findings/GCC-000N.md
  and a per-finding UPSTREAM.md.
