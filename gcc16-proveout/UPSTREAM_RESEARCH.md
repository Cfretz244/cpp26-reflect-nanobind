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

## Filing notes (Phase 4)

- File on gcc.gnu.org/bugzilla, component **c++**, with `-freflection` in
  the keywords/summary; CC the reflection implementers (the PR120775
  series author and the active fixers seen above: Patrick Palka, Marek
  Polacek, Jakub Jelinek).
- Each filing mirrors the TC-XXXX discipline: minimized probe from
  gcc16-proveout/probes/ + expected-vs-actual + clang-p2996 cross-check;
  record the filed number in the corresponding corpus/findings/GCC-000N.md
  and a per-finding UPSTREAM.md.
