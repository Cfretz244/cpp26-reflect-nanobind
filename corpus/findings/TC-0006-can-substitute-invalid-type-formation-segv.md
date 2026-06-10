# TC-0006 — can_substitute/substitute SEGV when substitution forms an invalid type (reference to void)

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project. The null `Spec` returned by `InstantiateFunctionDeclaration`
  (SFINAE-context failure) was dereferenced in
  `MetaActionsImpl::Substitute(FunctionTemplateDecl*…)`; the alias path leaked
  a hard error out of `CheckTemplateIdType`; the var path tripped the
  post-`Substitute` assert. Fix: the `MetaActions::Substitute` overloads gain a
  `SuppressDiagnostics` flag + null-on-failure contract, and the `substitute`
  metafunction maps null to `can_substitute == false` (NoDiagnose) or a new
  `metafn_substitution_failed` note. Regression tests
  `can-substitute-invalid-type-formation.pass.cpp` + a diagnosing case in
  `substitute.verify.cpp`. Upstream: `repros/TC-0006/UPSTREAM.md`.
- **Track:** toolchain (clang-p2996, pinned by this repo; same family as the
  bloomberg fork base `837da39eb88c`)
- **Found via:** `corpus/runs/expected` (TartanLlama/expected v1.3.1). Putting
  `^^tl::expected<void, std::string>` in the `reflect_` pack crashed the compiler
  (exit 139) while instantiating `check_stl_casters` — i.e. while the binder's
  caster/spec walks probed member templates with `can_substitute({})`.
- **Repro:** `corpus/findings/repros/TC-0006/repro.cpp` (standalone, no nanobind,
  no library; `-fsyntax-only` suffices).

## Symptom

`std::meta::can_substitute(tmpl, args)` — the SFINAE-probe metafunction that is
*supposed* to answer false for failing substitutions — SIGSEGVs the clang
frontend when the substitution forms an invalid type inside a template-id.
The minimal shape:

```cpp
template <class T> struct trait { using type = void; };
template <class OT> typename trait<OT&>::type f() {}

static_assert(!std::meta::can_substitute(^^f, std::vector{^^void}));   // SIGSEGV
static_assert( std::meta::can_substitute(^^f, std::vector{^^int}));    // fine
```

Substituting `OT=void` must form `trait<void&>` — reference-to-void, the
canonical immediate-context deduction failure that every `enable_if`-era SFINAE
library leans on. Expected: substitution failure ⇒ `can_substitute` returns
`false`. Actual: stack dump ending in

```
(anonymous namespace)::MetaActionsImpl::Substitute(clang::FunctionTemplateDecl*, ...)
clang::substitute(...)  <- clang::Metafunction::evaluate <- ReflectionEvaluator
```

The defaulted-parameter member-template form (the field shape) crashes the same
way: `template <class OT = T> typename trait<OT&>::type swap(Exp&)` inside
`Exp<T>`, probed with `can_substitute(member, {})` on `Exp<void>`.

## Field shape

`tl::expected<void, E>`'s member

```cpp
template <class OT = T, class OE = E>
detail::enable_if_t<detail::is_swappable<OT>::value && ...> swap(expected&) noexcept(...);
```

hides the reference-to-void formation behind `detail::is_swappable<OT=void>`
(tl's C++11 `swap_adl_tests` machinery: `decltype(swap(std::declval<T&>(), ...))`).
Ordinary C++ is fine with all of it — `detail::is_swappable<void>::value` is
simply `false`, and `e.swap(f)` on `expected<void,E>` SFINAEs the member away.
But the nanobind binder probes every member template with `can_substitute({})`
(`fn_template_default_instantiable`) to decide default-instantiability, so
MERELY REFLECTING over `expected<void, E>`'s members crashes the compiler.

Notes from minimization:

- Order-independent and not dodged by pre-instantiating the class first (unlike
  TC-0007): `tl::expected<void, std::string> inst;` before the probe still
  crashes.
- `decltype(std::declval<OT&>())` as a template-parameter default argument does
  NOT crash (reports failure correctly); the invalid type must be formed inside
  a template-id in the declaration (return type / `enable_if` nested-name).
- Not specific to member templates: a free function template with an explicit
  `^^void` argument crashes identically.

## Impact on the corpus

`corpus/runs/expected` originally dropped `^^tl::expected<void, std::string>`
from its bind set over this (and TC-0007). With both fixed, the void spec is
back in the pack and the run is at outcome E: the binder's
`can_substitute({})` probes on `swap<OT=void>`/`value<U=void>` now report
not-substitutable and the members are gracefully skipped — exactly the
SFINAE contract. Any library with `enable_if`-constrained member templates
over a possibly-void parameter had the same exposure.
