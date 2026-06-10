# TC-0005 — function-type metafunctions blind to AttributedType sugar: `[[clang::lifetimebound]]` methods misreport all qualifiers; their type is rejected by `return_type_of`/`parameters_of`

(Promoted from TC-0003's addendum #2, where it was recorded as
"`is_rvalue_reference_qualified(underlying_entity_of(proxy))` misreports `false` for
`&&`-qualified members reached through a proxy on an instantiated class template".
Minimization showed proxies are **incidental**: the trigger is
`ABSL_ATTRIBUTE_LIFETIME_BOUND` = `[[clang::lifetimebound]]` on the methods, and the
misreport reproduces on plain direct reflections of a non-template class.)

- **Status:** FIXED locally (llvm-project @ `06b9344`,
  `clang/lib/AST/ExprConstantMeta.cpp`); **upstreamed**: issue
  [bloomberg/clang-p2996#292](https://github.com/bloomberg/clang-p2996/issues/292) + PR
  [#293](https://github.com/bloomberg/clang-p2996/pull/293) (see
  `repros/TC-0005/UPSTREAM.md`). Standalone repro: `repros/TC-0005/repro.cpp`.
  Regression test:
  `llvm-project/libcxx/test/std/experimental/reflection/attributed-function-type-queries.pass.cpp`.
- **Kind:** silent wrong answers from consteval predicates (worse class than an ICE) +
  wrong rejection of valid type queries.
- **Found via:** binding `absl::StatusOr<int>` — every qualifier predicate reported
  `false` for all four `value()` overloads (`const&`/`&`/`const&&`/`&&`), observed
  through their using-redeclaration proxies (hence the original proxy attribution).
- **Triage:** `toolchain`. dedup_key: `attributed-fn-type-sugar-blind-predicates`.

## Root cause

An attribute on a function declarator — e.g. `[[clang::lifetimebound]]` applied to the
implicit object parameter, which Abseil puts on every accessor via
`ABSL_ATTRIBUTE_LIFETIME_BOUND` — wraps the declaration's `FunctionProtoType` in
`AttributedType` sugar. Several metafunctions reached the `FunctionProtoType` with a
sugar-blind `dyn_cast`, which fails on the wrapper:

| metafunction | misbehavior on a lifetimebound method |
|---|---|
| `is_const` / `is_volatile` (via `isConstQualifiedType`/`isVolatileQualifiedType`) | silently `false` |
| `is_lvalue_reference_qualified` / `is_rvalue_reference_qualified` (both the Declaration and Type arms) | silently `false` |
| `return_type_of` / `get_ith_parameter_of` (`parameters_of`) / `has_ellipsis_parameter` on the function's **type** (`type_of(m)` keeps the sugar) | rejected as not-a-function-type → "not a constant expression" |
| `is_noexcept` | always worked — `isFunctionOrMethodNoexcept` checks `isFunctionProtoType()` (canonical) and desugars via `getAs` — the in-file precedent for the fix |

`is_function` was unaffected (checks the decl kind), which is why the original probe
showed `is_function=true` with all qualifiers `false`.

Fix: replace each `dyn_cast<FunctionProtoType>(QT)` with `QT->getAs<FunctionProtoType>()`
(desugars; returns null for genuine non-function types, preserving the existing
behavior for every other kind). Seven sites in `ExprConstantMeta.cpp`.

## Ruled out during minimization (from the TC-0003 addendum work)

- Entity proxies / `underlying_entity_of`: the misreport reproduces identically on
  direct member reflections; proxy chains, injected-class-name qualifiers
  (`using StatusOr::OperatorBase::value;`), multi-level private bases, and
  instantiated-class-template shapes all answer correctly once the attribute is
  removed — and all misreport with it, proxies or not.
- The TC-0004 mangling fold (re-probed then; orthogonal, as suspected).

## Binder aftermath

The binder never trusted the qualifier predicates here — its supported-qualifier gate
is "does a `reflect_method_binder` partial specialization exist for this exact function
type" (`sizeof` on the undefined primary), which is the same mechanism every binding
path uses (the binder matrix is the single source of truth; nothing to remove). The
stale "predicates untrustworthy on proxy underlyings (TC-0003 addendum, open)" comments
in `nb_reflect.h` and `nanobind/CLAUDE.md` were rewritten to reflect the real root
cause and its fix.

## Validation

- `repros/TC-0005/repro.cpp`: six qualifier `static_assert`s fail at base
  `837da39eb88c`; `-DTYPE_QUERIES` adds the `return_type_of`/`parameters_of`/
  `has_ellipsis_parameter` not-a-constant-expression failures. All pass with the fix.
  Dropping the attribute makes base pass too (the control).
- StatusOr field probe (real Abseil): all four `value()` proxies' underlyings report
  `const&`/`&`/`const&&`/`&&` correctly with the fix (previously all-false).
- `attributed-function-type-queries.pass.cpp` passes (manual run); `entity-proxies` +
  `entity-proxy-member-queries` still pass; `clang/test/Reflection` lit 16/16; binder
  suite 50/50 with recompiled modules; corpus `abseil_statusor` + `json` gates at E.
