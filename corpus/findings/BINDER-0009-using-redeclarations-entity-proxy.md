# BINDER-0009 — using-redeclared base members are invisible without entity-proxy reflection

- **Status:** FIXED — the binder requires `-fentity-proxy-reflection` and binds
  using-redeclarations through their entity proxies (`reflect_bind_proxy`): re-exports from
  private/protected bases bind by splice-calling THROUGH the proxy (a public member of the
  derived class); public-base re-exports are recognized as flattening-covered and skipped
  (no duplicate overloads). `absl::StatusOr<T>::value()` now binds head-on. Landing this
  surfaced THREE toolchain bugs (metafunction ICEs, a mangler ICE, a qualifier misreport)
  — see TC-0003. Template and data-member re-exports from inaccessible bases stay
  unsupported (documented).
- **Found via:** `corpus/runs/abseil_statusor`. `absl::StatusOr<T>` declares `value()` /
  `operator*` / `operator->` in the **private** base `internal_statusor::OperatorBase<T>`
  (statusor_internal.h:477+) and re-exports them publicly with `using
  StatusOr::OperatorBase::value;` (statusor.h:504).
- **Files:** `nanobind/include/nanobind/nb_reflect.h` (member walk), binder build flags.

## The facts (all verified empirically on the pinned toolchain)

1. Member **type aliases** (`typedef`/`using X = ...`) ARE enumerable via `members_of`
   (`is_type_alias == true`, named). Not a gap.
2. Under plain `-freflection-latest`, a `using Base::f;` shadow declaration produces **no
   entry** in `members_of(^^Derived, access_context::unchecked())`. With a private base,
   the member is therefore unreachable: the base's own `f` can't be called through Derived
   from outside (inaccessible base path) — the using-declaration is precisely what grants
   access, and it is the invisible part.
3. With the fork's **`-fentity-proxy-reflection`** (NOT implied by `-freflection-latest` —
   verified; the llvm-project CLAUDE.md table previously claimed otherwise), the shadow
   declaration IS enumerated: named `f`, `is_entity_proxy(m) == true`,
   `is_function(m) == false`; `underlying_entity_of(m)` / `proxied_entity_of(m)` resolve it.
   Driver gate: `ExprConstantMeta.cpp:1456` (`isa<UsingShadowDecl>(D) &&
   !EntityProxyReflection -> not enumerable`).

## Fix direction

Add `-fentity-proxy-reflection` to the binder's required flags; in `bind_class_contents`
(and the flattening/member-template walks), recognize `is_entity_proxy(m)`, resolve via
`underlying_entity_of`, and bind through the proxy (it is a public member of the derived
class, so access is correct even for private-base re-exports). Investigate splice behavior
(`self.[:proxy:](...)` vs splicing the underlying function) during implementation.

## Workaround in the corpus

`abseil_statusor` reaches E with `get_int/get_str/get_dbl` fixture accessors that call the
real `value()` (including the real `BadStatusOrAccess` throw path). Once proxy support
lands, swap the fixtures for the direct binding. dedup_key: `using-shadow-entity-proxy`.
