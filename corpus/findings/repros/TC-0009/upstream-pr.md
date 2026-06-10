Fixes #ISSUE_TC9.

> **Stacked on #287 and the deduction-guide PR (#PR_TC8)** — the first two commits are theirs (this change amends the same `mangleReflection` hash block); review the last commit only.

## Problem

The NTTP discriminator for overloaded function-template reflections (#286 / #287) hashes the template parameter list plus the declaration pattern via `ODRHash::AddFunctionDecl(pattern, /*SkipBody=*/true)`. But `AddFunctionDecl` **silently no-ops for any declaration in "specialization context"** — its decl-context walk returns on finding a `ClassTemplateSpecializationDecl`. A member template of an instantiated class template — the common `members_of` shape — therefore contributed only its template HEAD to the hash, and same-named siblings with **identical heads** still mangled identically. #287's own field shape (absl `operator[]` + its pack twin) escaped only because those heads differ.

Field shape: `tl::expected<T,E>`'s four `value()` member templates (`const&`/`&`/`const&&`/`&&`, one shared `template <class U = T, enable_if_t<!is_void<U>::value>* = nullptr>` head) — all four reflections got one mangled name, CodeGen silently folded the linkonce_odr dispatcher specializations of a reflection-driven binding generator, and `value()` never bound. Caught by a differential test suite, not a diagnostic; where the TU collides explicitly the symptom is `error: definition with same mangled name ... as another definition` (see #ISSUE_TC9 for the reproducer).

## Fix

`clang/lib/AST/ItaniumMangle.cpp`, same hash block: additionally hash what `AddFunctionDecl` skips in specialization context —

- the pattern's function type via `ODRHash::AddQualType` (return type, parameter types, cv-quals). The ODR *hash* handles the dependent pattern types that a structural *mangling* of the type cannot (lifetimebound SFINAE, `noexcept(...)` referencing parameters — the original #287 constraint);
- the ref-qualifier, which even `ODRHash`'s `VisitFunctionProtoType` omits — two siblings can differ in nothing else.

## Test

`libcxx/test/std/experimental/reflection/fn-template-nttp-mangling-spec-context.pass.cpp` (new): the four-sibling `value()` shape (identical heads, cv/ref-qualifier + return-type differences) and a ref-qualifier-only pair, each pinned as `&probe<m>` NTTPs from a `members_of` walk over the *specialization* and asserted pairwise distinct at runtime.

## Validation

- Base: the deduction-guide PR's branch tip (itself on #287 on `837da39eb88c`); builds standalone.
- Without the fix: the #ISSUE_TC9 reproducer fails at `-c` with the duplicate-mangled-name error (clean at `-fsyntax-only`; clean with the same signatures at namespace scope — the no-specialization-context control); the new test fails.
- With the fix: reproducer matrix clean; new test passes; #287's `substitute-nested-dependent.pass.cpp` and the deduction-guide test still pass on the stack tip.
- `clang/test/Reflection`: 16/16 with the fix on this machine — identical to base.
- Downstream soak (reflection-driven nanobind binding generator): 53-test binder suite green; the TartanLlama/expected v1.3.1 corpus run that caught this (its `value()` differential went from `AttributeError` to passing) is green end-to-end, `tl::expected<int,std::string>::value()` bound with all four sibling reflections mangled distinctly.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
