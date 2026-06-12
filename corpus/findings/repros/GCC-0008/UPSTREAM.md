# GCC-0008 upstream record — function-template reflection NTTPs mangle identically

- status: **PATCH READY** (not yet filed/posted — filing is the user's call)
- repro: `gcc16-proveout/probes/xfail_gcc8_member_template_nttp_mangling.cpp`
  (self-contained, ~60 lines; attach verbatim)
- patch: `0001-cxx-reflection-mangle-function-template-reflection-NTTPs.patch`
  (in this directory; also commit `be4033289d7` on the devenv checkout's
  `proveout-fixes` branch, based on master `7ce3a7b1beb`, 2026-06-12)
- affected: GCC 16.1 (release), trunk 17.0 @ 7ce3a7b1beb (verified both)

## Root cause

`write_reflection` (gcc/cp/mangle.cc) encodes a Template-kind reflection of a
function template as `ft [<prefix>] <unqualified-name>` — scope + name only.
Class/variable/alias templates and concepts cannot overload, so name+scope
identifies them; **function templates overload**. Reflections of two
same-named sibling templates (const/non-const member pair; namespace-scope
siblings differing in head, signature, or constraints) therefore mangle
identically. A dispatcher template instantiated once per sibling with the
reflection as an NTTP emits two definitions under ONE symbol:

- 16.1 release builds: `Error: symbol '_Z8dispatchI5TableLDmftS0_2atEEvRT_Ri'
  is already defined` (assembler hard error), or with comdat linkage a
  **silent fold** where one body serves both call sites (the dangerous mode).
- trunk `--enable-checking` builds: upgraded to a symtab verifier ICE,
  "Two symbols with same comdat_group are not linked by the
  same_comdat_group list" — proving the bug at the compiler layer.

This is stock GCC's instance of the same design gap the clang-p2996 fork had
(its TC-0004/TC-0009, fixed there by folding an ODR hash of template head +
pattern type into the mangling).

## The fix

Extend the (GCC-invented, pre-ABI-standardization) `ft` encoding with the
template head in the style of lambda template heads
(`<template-param-decl>*` + the head's constraints via
`write_tparms_constraints`), a `_` separator, the type of the templated
function (`write_type` on the pattern's FUNCTION/METHOD_TYPE — this is what
separates the const/non-const pair, via the implicit object type), and any
trailing requires-clause (`Q <constraint-expression>`). Example:
`LDmft3TFnEE` → `LDmft3TFnTnDa_FvvEE`.

Only `ft` changes; all other reflection kinds keep their encoding (they
cannot collide). ABI note for the submission: reflection NTTP mangling is new
in GCC 16 and not yet in the Itanium ABI document; the old encoding is
*ambiguous* (ill-formed-no-diagnostic / wrong-code), so this is a mangling
bug fix, not an ABI break of well-defined behavior. An `abi_check`-style
compat gate can be added if the maintainers want one.

## Verification (devenv, trunk @ 7ce3a7b1beb + patch)

- `xfail_gcc8_member_template_nttp_mangling.cpp`: compiles, links, runs,
  exit 0 (was: assembler error / checking ICE).
- Reflection + constexpr testsuite slices (`reflect/*`, `cpp26/reflection*`,
  `cpp26/annotations*`, `cpp26/splice*`, `cpp26/expansion-stmt*`,
  `cpp26/consteval-prop*`, `cpp26/constexpr-*`): 3939 passes, 0 unexpected
  failures (only the two stale mangle1.C expectations needed updating —
  included in the patch).
- New regression test `g++.dg/reflect/mangle8.C` (dg-do run: catches both the
  assembler-error and the silent-fold mode); all 157 `reflect/mangle*` tests
  pass.
- Probe conformance smoke `gcc16-proveout/probes/0*.cpp`: unchanged.

## Bugzilla material (component c++, keywords wrong-code)

- Summary: `[C++26] -freflection: reflections of same-named function
  templates as NTTPs mangle identically (silent comdat fold or "symbol
  already defined")`
- Description: paragraphs 1–2 of "Root cause" above + expected/actual +
  cross-check: clang-p2996 after its PR #287/#301 mangles the siblings
  distinctly; the probe runs and exits 0 there.
- Attach: the probe; mention trunk checking-build signature; offer the patch
  (gcc-patches with this directory's patch; ChangeLog included).
- Cite as area neighbor: PR 123237 (write_type ICE mangling dependent
  splices, fixed r16-8592) — different shape, same code region.
- CC: reflection implementers (PR120775 series author; Patrick Palka, Marek
  Polacek, Jakub Jelinek).
