# TC-0010 — reflections of a re-opened namespace compare unequal across redeclarations

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project (`clang/lib/AST/APValue.cpp`, `profileReflection`,
  `ReflectionKind::Namespace`); regression test
  `llvm-project/libcxx/test/std/experimental/reflection/namespace-reflection-equality-reopened.pass.cpp`;
  upstreamed as bloomberg/clang-p2996 issue #302 / PR #305 (tracking issue #308)
- **Track:** toolchain (clang-p2996, pinned by this repo), reflection equality
  (constant evaluator / APValue profiling)
- **Found via:** `corpus/runs/eigen` (Eigen 5.0.1) — the run's
  `nb::exclude_<^^Eigen::internal>` namespace exclusion (the new binder
  feature this run introduced) silently failed to match
  `Eigen::internal::pointer_based_stl_iterator<...>`: the discovery probe kept
  printing internal iterator specs in the bind set.
- **Repro:** `corpus/findings/repros/TC-0010/repro.cpp` (standalone,
  `-fsyntax-only` suffices — it is a constant-evaluation wrong-answer, not a
  crash).

## Symptom

P2996: two reflections compare equal iff they designate the same entity. A
namespace is ONE entity regardless of how many times it is re-opened. But:

```cpp
namespace c { namespace d {} }
namespace c { namespace d { template <class T> struct R {}; } }  // re-opened

static_assert(std::meta::parent_of(^^c::d::R) == ^^c::d);   // FAILS
```

`^^c::d` wraps the namespace's FIRST `NamespaceDecl`; `parent_of(^^c::d::R)`
wraps the `NamespaceDecl` of the re-opened block that declared `R`. Reflection
equality goes through `APValue::Profile` → `profileReflection`
(`clang/lib/AST/APValue.cpp`), whose `ReflectionKind::Namespace` case profiles
the raw opaque declaration pointer — different redeclarations profile
differently, so the reflections compare unequal. Single-block namespaces are
unaffected (only one decl exists), which is why this survived: every
`parent_of(...) == ^^ns` test in simple code passes.

The `ReflectionKind::Template` case in the same switch already canonicalizes
(`RedeclarableTemplateDecl::getCanonicalDecl()`) for exactly this reason —
`Namespace` was the gap.

## Fix

Profile the canonical declaration:

```cpp
case ReflectionKind::Namespace:
  ID.AddPointer(V.getReflectedNamespace()->getCanonicalDecl());
  return;
```

A `NamespaceAliasDecl` (also carried under `ReflectionKind::Namespace`)
canonicalizes to its own first declaration, NOT to the aliased namespace, so
the alias-vs-namespace distinction (`^^alias != ^^ns`,
`underlying_entity_of(^^alias) == ^^ns`) is preserved; the regression test
pins that too. `TranslationUnitDecl` (the global namespace) is its own
canonical decl.

## Field shape (how Eigen surfaced it)

Eigen re-opens `Eigen::internal` in nearly every header. The binder's
namespace-level call-site exclusion (`nb::exclude_<^^Eigen::internal>`,
introduced by the eigen corpus run) tests
`parent_of(template_of(spec)) == ^^Eigen::internal` while walking enclosing
scopes; before the fix that answered false for anything declared after the
first `namespace internal` block — i.e. essentially everything — and
`internal::pointer_based_stl_iterator<Matrix<...>>` (from
`DenseBase::begin()/end()`) leaked through the exclusion into the bind set.

## Validation

- `repro.cpp`: q2 static_assert fails at the pre-fix toolchain; all three pass
  with the fix.
- Regression test `namespace-reflection-equality-reopened.pass.cpp` compiled
  and run manually (the runtimes lit harness has no test-suite install on this
  machine): passes with the fix; its re-opened cases fail without.
- The eigen discovery probe: with the fix, `^^Eigen::internal` exclusion
  filters `pointer_based_stl_iterator` (and every other internal type) as
  intended.
- Binder suite: 54/54 after the toolchain rebuild.
