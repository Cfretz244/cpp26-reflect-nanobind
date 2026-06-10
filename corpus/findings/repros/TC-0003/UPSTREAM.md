# FILED — bloomberg/clang-p2996 issue + PR (2026-06-09)

- **Issue:** https://github.com/bloomberg/clang-p2996/issues/290
- **PR:** https://github.com/bloomberg/clang-p2996/pull/291 — branch
  `reflect-entity-proxy-hardening` on `Cfretz244/llvm-project` (commit `425d3d6`:
  `651ff7a` + `7eede09` squashed onto their `p2996` tip `837da39` == our local base,
  so the PR code is byte-for-byte what the corpus validated; internal TC-0003
  references absent from code comments).

The draft below is what was filed (lightly adapted in the issue itself).

## Validation evidence behind the filing (all on this laptop)

- `repro.cpp` here: each of the six metafunctions independently ICEs the base compiler
  (selectable by `-DPROBE_*`; `UNREACHABLE executed at
  clang/lib/AST/ExprConstantMeta.cpp:{5240,5391,5435,4563,4830,4789}` at `837da39`).
  All six compile clean with the fix.
- `repro_mangle.cpp` here: default shape (operator shadow, NO class template needed)
  crashes the base compiler with the `cast<>` assertion in
  `CXXNameMangler::mangleUnqualifiedName`; `-DCOLLIDE` (one using-declarator over a
  four-overload set) errors with `definition with same mangled name
  '_Z5probeIMaN1SIiE5valueE$EEiv'`. Both compile and run (exit 0) with the fix.
- The field shape (probe over `members_of(^^absl::StatusOr<int>)` proxies as NTTPs
  against real Abseil) crashes the base compiler with the same
  `mangleUnqualifiedName` assertion — StatusOr re-exports `operator*`/`operator->`
  via `using StatusOr::OperatorBase::...`. The original finding mis-attributed the
  crash to the shadow living in a class template specialization; minimization showed
  the operator NAME is the trigger and a plain class suffices.
- Regression test
  `libcxx/test/std/experimental/reflection/entity-proxy-member-queries.pass.cpp`:
  ICEs at base (first metafunction arm), passes with the fix; asserts all five proxy
  NTTP instantiations (4 same-named overload shadows + operator shadow) are pairwise
  distinct and distinct from their underlying declarations' instantiations.
- PR-branch compiler (fix on plain `837da39`, no other local commits):
  repros + regression test all pass; `clang/test/Reflection` 15/16 — identical to
  pristine base (the one failure, `splice-exprs.cpp`, is the fork's documented
  pre-existing sharp edge, unrelated to this change).
- Full local toolchain (reflection-p2996): lit 16/16, binder suite 50/50 with modules
  recompiled, corpus `abseil_statusor` and `json` gates at outcome E.

---

## Title

Entity-proxy reflections crash member-kind metafunctions and the Itanium mangler
(`-fentity-proxy-reflection`)

## Summary

With `-fentity-proxy-reflection`, `members_of` enumerates using-shadow declarations as
`ReflectionKind::EntityProxy` — but several consumers of reflections still treat that
kind as `llvm_unreachable("proxies should already have been unwrapped")`, so ordinary
consteval code that enumerates members and asks ordinary questions ICEs:

1. **Member-kind metafunctions** (`clang/lib/AST/ExprConstantMeta.cpp`): six of them
   crash when asked of a proxy — `is_constructor`, `is_destructor`,
   `is_special_member_function`, `is_static_member`, `is_enumerable_type`,
   `has_complete_definition`. The surrounding kind predicates already classify
   proxies gracefully (they answer `false`, e.g. `is_function`, `is_variable`,
   `is_type`, `is_complete_type`); these six arms are the outliers.

2. **Itanium mangling of a proxy as a template argument**
   (`clang/lib/AST/ItaniumMangle.cpp`, `mangleReflection`,
   `ReflectionKind::EntityProxy`): the proxy is encoded by mangling the shadow
   declaration's *name* via `mangleNameWithAbiTags`, with two defects:
   - an **operator-named shadow** (`using B::operator*;`) crashes outright —
     `mangleUnqualifiedName` casts the decl to `FunctionDecl` to disambiguate the
     operator name, and a `UsingShadowDecl` is not one (`cast<>` assertion);
   - **one using-declarator over an overload set** introduces several same-named
     shadows whose proxy reflections all mangle identically — `error: definition
     with same mangled name ... as another definition` at best, a silent
     linkonce_odr fold (the #286 failure mode) at worst.

## Field evidence

A reflection-driven binding generator walks `members_of(^^T)` and (a) asks each entry
the ordinary kind questions (its constructor pass calls `is_constructor` on every
member — instant ICE on the first re-exported name), and (b) forwards selected members
to helpers as `std::meta::info` NTTPs. `absl::StatusOr<T>` re-exports its accessors
from a private base (`using StatusOr::OperatorBase::value;` /
`...::operator*;` / `...::operator->;`): the operator shadows crash the mangler, and
the four-overload `value()` set collides. (We initially mis-read the mangler crash as
requiring the shadow to live in a class template specialization; the operator name is
the actual trigger — a plain class reproduces.)

## Reproducers

(both self-contained; `-std=c++26 -freflection-latest -fentity-proxy-reflection`)

### 1. Metafunction ICEs (`-fsyntax-only` suffices)

```cpp
#include <experimental/meta>
namespace meta = std::meta;

struct B {
  int f() const { return 1; }
  static int g() { return 2; }
  static int s;
  int field;
  using T = int;
  struct N {};
};
struct D : private B {
  using B::f; using B::g; using B::s; using B::field; using B::T; using B::N;
};

consteval bool query_proxies() {
  for (auto m : meta::members_of(^^D, meta::access_context::unchecked())) {
    if (!meta::is_entity_proxy(m)) continue;
    if (meta::is_constructor(m)) return false;   // ICE on the first proxy
  }
  return true;
}
static_assert(query_proxies());
int main() {}
```

Each of `is_destructor` / `is_special_member_function` / `is_static_member` /
`is_enumerable_type` / `has_complete_definition` substituted for `is_constructor`
independently produces the same crash (different line).

### 2. Mangler ICE (must reach CodeGen; `-fsyntax-only` does NOT reproduce)

```cpp
#include <experimental/meta>
namespace meta = std::meta;

struct OB { int operator*() const { return 1; } };
struct S : private OB { using OB::operator*; };

template <std::meta::info M> int probe() { return 7; }

int main() {
  template for (constexpr auto m : std::define_static_array(
          meta::members_of(^^S, meta::access_context::unchecked()))) {
    if constexpr (meta::is_entity_proxy(m))
      probe<m>();
  }
}
```

Replace `OB`/`S` with a four-overload `value()` set behind one `using OB<T>::value;`
to get the collision variant instead of the crash.

## Expected

Compiles (and for #2, runs). The metafunctions answer `false` for a proxy — a shadow
declaration is never itself a constructor/destructor/etc.; the underlying entity's
properties are available through `underlying_entity_of`. Each proxy NTTP gets its own
specialization.

## Actual (at `837da39eb88c`)

1. `proxies should already have been unwrapped` / `UNREACHABLE executed at
   clang/lib/AST/ExprConstantMeta.cpp:5240` (line varies by metafunction).
2. `Assertion failed: (isa<To>(Val) && "cast<Ty>() argument of incompatible type!")`
   in `CXXNameMangler::mangleUnqualifiedName`; the overload-set variant instead emits
   `error: definition with same mangled name '_Z5probeIMaN1SIiE5valueE$EEiv' as
   another definition`.

## Suggested fix (PR follows)

- The six metafunction arms join the graceful-`false` case-lists, matching the
  surrounding predicates. The remaining `EntityProxy` unreachable arms in
  `ExprConstantMeta.cpp` were probed NOT reachable from user code (`identifier_of` /
  `has_identifier` / `source_location_of` pre-unwrap via `MaybeUnproxy`;
  `substitute`'s dispatcher unwraps template arguments before `TArgFromReflection`;
  `reflect_invoke` through a proxy fails gracefully) and are left as-is.
- `mangleReflection`'s `EntityProxy` case mangles the proxy's TARGET declaration
  kind-aware (ctors/dtors via `GlobalDecl`, functions/variables/fields via
  `mangle()`, enum constants as literals, types via `mangleCanonicalTypeName`, else
  the source name), keeping the `a` tag so a proxy-of-X reflection remains a distinct
  template argument from a declaration-of-X reflection. This also discriminates the
  same-named shadows of an overload set (each mangles via its own target overload).

Regression test included:
`libcxx/test/std/experimental/reflection/entity-proxy-member-queries.pass.cpp` —
consteval sweep of all six metafunctions over proxies of every member kind, plus
runtime distinctness assertions for the proxy-NTTP manglings (operator shadow +
four-overload shadow set; `static_assert` cannot catch a mangling fold).

## Possibly related

The qualifier-predicate misreport through `StatusOr<int>::value()` proxies
(`is_rvalue_reference_qualified(underlying_entity_of(p))` returning `false` for all
four overloads) mentioned in #286's "Possibly related" is a distinct issue — not
fixed by this change — and will be reported separately once minimized.
