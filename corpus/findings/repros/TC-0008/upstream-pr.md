Fixes #298.

> **Stacked on #287** — the first commit here is #287's (this change extends the same `mangleReflection` hash block); review the last commit only.

## Problem

`members_of` over a namespace enumerates deduction guides like any other member, and lifting that member list into `define_static_array` mangles each element reflection as a template argument of the backing array specialization. `mangleReflection`'s `Template` case encodes a reflected template via `mangleTemplateName` → `mangleUnqualifiedName`, which has no case for `CXXDeductionGuideName`: `llvm_unreachable("Can't mangle a deduction guide name!", ItaniumMangle.cpp:1774)` — a sound invariant for normal symbol mangling (guides are never odr-used) that reflection now makes reachable from ordinary user code. Field shape: TartanLlama/expected's namespace-scope `template <class E> unexpected(E) -> unexpected<E>;` — a reflection-driven binding generator walking the `tl` namespace ICE'd at codegen while binding ANY `tl` class (see #298 for the reproducer + build matrix).

## Fix

`clang/lib/AST/ItaniumMangle.cpp`, local to `mangleReflection`'s `ReflectionKind::Template` case (the `mangleUnqualifiedName` unreachable stays): a guide reflection encodes as `"dg"` + the *deduced* template's name + the same `'$'`-bracketed ODR-hash discriminator used for overloaded function templates (#287). Two subtleties force more than crash avoidance:

- every guide for one template shares the single `CXXDeductionGuideName`, and Sema's **implicit** guides (per-constructor + the copy guide) are enumerated alongside explicit ones once CTAD has been used in the TU;
- an implicit per-constructor guide can be **structurally identical** to a same-signature explicit guide (and the copy guide to a per-constructor guide for `X(X<E>)`), so the hash also folds in `isImplicit()` and the deduction-candidate kind.

Deterministic and cross-TU-stable (ODR hash), preserving legitimate linkonce_odr merging; distinct from a reflection of the deduced class template itself (no `dg` tag).

## Test

`libcxx/test/std/experimental/reflection/deduction-guide-reflection-mangling.pass.cpp` (new): the `define_static_array(members_of(...))` lift shape (the ICE), with **four** guides in the enumeration — two explicit (one structurally identical to the implicit per-constructor guide) + two implicit — pinned as `&probe<m>` NTTPs and asserted pairwise distinct at runtime, plus distinct from the deduced class template's own reflection. A fold is invisible to `static_assert` (the AST is always correct), so the observations are runtime.

## Validation

- Base: #287's branch tip (`caac14845397`, itself on `837da39eb88c`); builds standalone.
- Without the fix: the #298 reproducer ICEs at `-c` (clean at `-fsyntax-only` and without the guide); the new test ICEs.
- With the fix: reproducer matrix fully clean; new test passes; #287's own regression test (`substitute-nested-dependent.pass.cpp`) still passes on the stack.
- `clang/test/Reflection`: 16/16 with the fix on this machine — identical to base.
- Downstream soak (reflection-driven nanobind binding generator): 53-test binder suite green; TartanLlama/expected v1.3.1 corpus run — formerly ICE'ing at codegen on every `tl` class — passes its full differential suite.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
