# TC-0012 — is_complete_type blind to alias sugar: instantiable specs named through typedefs reported incomplete

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project (`clang/lib/AST/ExprConstantMeta.cpp`, `is_complete_type`);
  regression test
  `llvm-project/libcxx/test/std/experimental/reflection/is-complete-type-alias-sugar.pass.cpp`;
  upstreamed as bloomberg/clang-p2996 issue #304 / PR #307 (tracking issue #308)
- **Track:** toolchain (clang-p2996, pinned by this repo), metafunction engine —
  the TC-0005 theme (sugar-blind metafunctions) at a different site
- **Found via:** the eigen run's full-corpus regression sweep — the new
  completeness gate in the binder's spec discovery (BINDER-0014) silently
  dropped `spdlog::sinks::stdout_sink_base<console_mutex>`/`stdout_sink_mt`
  from the spdlog bind set: both reach the walk through ALIASES, and
  `is_complete_type` answered false for them.
- **Repro:** `corpus/findings/repros/TC-0012/repro.cpp` (standalone,
  `-fsyntax-only`).

## Symptom

```cpp
template <class T> struct Box { T v; };
using BoxInt = Box<int>;                              // nothing else names Box<int>
static_assert(std::meta::is_complete_type(^^BoxInt)); // FAILS
static_assert(std::meta::is_complete_type(^^Box<long>)); // passes (direct)
```

`is_complete_type` is implemented as "instantiate if a definition is
reachable, then test completeness":

```cpp
if (Decl *typeDecl = findTypeDecl(RV.getReflectedType()))
  (void) Meta.EnsureInstantiated(typeDecl, Range);
result = !RV.getReflectedType()->isIncompleteType();
```

but it passes the SUGARED type to `findTypeDecl`, which for a `TypedefType`
returns the alias declaration — `EnsureInstantiated(TypedefDecl)` instantiates
nothing, and a never-yet-referenced specialization then reads as incomplete.
The answers are order-dependent: naming the same spec directly first
instantiates it, after which the alias query flips to true. The `members_of`
family desugars aliases before `findTypeDecl` (`desugarType(...,
UnwrapAliases=true, ...)`); `is_complete_type` was the gap.

## Fix

Desugar exactly as `members_of` does before `findTypeDecl`, and test
completeness on the desugared type:

```cpp
QualType QT = desugarType(RV.getReflectedType(), /*UnwrapAliases=*/true,
                          /*DropCV=*/false, /*DropRefs=*/false);
if (Decl *typeDecl = findTypeDecl(QT))
  (void) Meta.EnsureInstantiated(typeDecl, Range);
result = !QT->isIncompleteType();
```

(`has_complete_definition` has a related sugar-blindness but ALSO ignores
not-yet-instantiated definitions by design; its semantics were left untouched.)

## Field shape

spdlog's reflect pack seeds through `stdout_sink_mt` (an alias for
`stdout_sink<console_mutex>`) and its sink chain reaches signatures through
member typedefs. The binder's discovery, newly completeness-gated by the eigen
run (a spec that cannot be completed can be neither bound nor walked —
BINDER-0014), asked `is_complete_type` about those alias-sugared specs, got
false, and silently dropped them: spdlog's sink hierarchy lost its
intermediate Python base. The binder ALSO now dealiases before its
completeness/exclusion gates (working on unpatched toolchains), so the fix has
two independent layers.

## Validation

- `repro.cpp` q1 fails at the pre-fix toolchain; all pass with the fix.
- Regression test `is-complete-type-alias-sugar.pass.cpp` (alias, member
  typedef, forward-declared-only, order-dependence, non-template controls).
- spdlog gates back to E; full corpus 20/20 E on the fixed toolchain.
