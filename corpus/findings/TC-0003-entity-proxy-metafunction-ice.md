# TC-0003 — member-kind metafunctions ICE on entity proxies (using-shadow declarations)

- **Status:** FIXED locally (llvm-project submodule); **upstreamed**: issue
  [bloomberg/clang-p2996#290](https://github.com/bloomberg/clang-p2996/issues/290) + PR
  [#291](https://github.com/bloomberg/clang-p2996/pull/291) (see
  `repros/TC-0003/UPSTREAM.md` for the validation evidence). The upstreaming pass
  (llvm-project @ `7eede09`) probed the remaining unreachable arms, hardened two more
  (`is_enumerable_type`, `has_complete_definition` — both ICE'd from valid user code),
  added the regression test the original fix lacked
  (`libcxx/test/std/experimental/reflection/entity-proxy-member-queries.pass.cpp`), and
  **corrected the mangler root cause** (see Addendum below). Standalone repros:
  `repros/TC-0003/repro.cpp` (six metafunctions, `-DPROBE_*` selectable) and
  `repros/TC-0003/repro_mangle.cpp` (operator-shadow ICE + `-DCOLLIDE` overload-set
  collision).
- **Found via:** adding `-fentity-proxy-reflection` to the binder for BINDER-0009
  (binding `using Base::f;` re-exports). The binder's constructor pass calls
  `std::meta::is_constructor(mem)` on every `members_of` entry; on a proxy entry the
  compiler aborts: `proxies should already have been unwrapped` — `llvm_unreachable`
  in `clang/lib/AST/ExprConstantMeta.cpp:5240` (`is_constructor`).
- **Triage:** `toolchain`. dedup_key: `entity-proxy-metafn-unreachable`.

## Repro (minimized; crashes before the fix, compiles after)

```cpp
// clang++ -std=c++26 -freflection-latest -fentity-proxy-reflection -fsyntax-only repro.cpp
#include <experimental/meta>
struct B { int f() const { return 1; } };
struct D : private B { using B::f; };
int main() {
    template for (constexpr auto m : std::define_static_array(
            std::meta::members_of(^^D, std::meta::access_context::unchecked()))) {
        constexpr bool b = std::meta::is_constructor(m);   // ICE on f's proxy
        (void) b;
    }
}
```

## Analysis

With `-fentity-proxy-reflection`, `members_of` (correctly) enumerates using-shadow
declarations as `ReflectionKind::EntityProxy`. But several metafunction switch
statements treat that kind as unreachable instead of answering the query, so any
consumer that enumerates members and asks an ordinary kind-predicate question ICEs.
All sites asserting `proxies should already have been unwrapped` in
`ExprConstantMeta.cpp` (line numbers at the pinned commit):

| metafunction | proxy behavior |
|---|---|
| `is_constructor` (5240) | **fixed** → `false` (a shadow decl is not itself a ctor) |
| `is_destructor` (~5394) | **fixed** → `false` |
| `is_special_member_function` (~5438) | **fixed** → `false` |
| `is_static_member` (~4563) | **fixed** → `false` |
| `identifier_of` (~2649) / `has_identifier` (~2745) | left as-is — they pre-unwrap via `MaybeUnproxy` (ExprConstantMeta.cpp:1653) before the switch, so the unreachable arm is genuinely unreachable (verified by reading the entry paths during the upstreaming pass; `source_location_of` does the same) |
| `has_complete_definition` (~4789), `is_enumerable_type` (~4830) | **fixed in the upstreaming pass** (llvm-project @ `7eede09`) → `false`. Probing showed both evaluate the user's argument directly into the kind switch with NO pre-unwrap, so `has_complete_definition(proxy)` / `is_enumerable_type(proxy)` ICE'd from valid user code exactly like the four above. (`is_complete_type` never crashed: it tests `isReflectedType()` instead of switching, i.e. graceful false — the consistency precedent.) |
| `reflect_invoke` (~7112) | left as-is — probed: invoking through a proxy now fails gracefully as not-a-constant-expression (no ICE; the `MaybeUnproxy` calls in its argument/function unwrapping paths keep the unreachable arm unreached). `substitute` with a proxy template argument is likewise safe (dispatcher unwraps before `TArgFromReflection`). |

The four fixed predicates return `false` for `EntityProxy`, consistent with how e.g.
`is_literal_operator` (an `isReflectedDecl()`-style check) already degrades
gracefully. Inconsistent handling across metafunctions is the underlying issue; an
upstream issue should propose a uniform policy (answer the query on the proxy as if
asked of the shadow declaration, or transparently unwrap).

## Addendum: two more entity-proxy toolchain bugs (same flag, found minutes later)

1. **Itanium mangler ICE on proxy reflections as template arguments.**
   `reflect_bind_proxy<T, ^^proxy>` makes the proxy an NTTP; mangling it went
   through `mangleNameWithAbiTags(shadow-decl)`, which trips a `cast<>` assertion,
   and the first fix attempt (`mangle(Target)`) hit the `mangle(GlobalDecl)`
   "unexpected kind of global decl" UNREACHABLE for non-function/variable targets.
   **Fixed locally** (`ItaniumMangle.cpp`, `ReflectionKind::EntityProxy` case):
   mangle the proxy's TARGET declaration kind-aware (functions/vars via `mangle`,
   enum constants as literals, types via `mangleCanonicalTypeName`, else the source
   name), keeping the `a` tag so proxy-of-X stays distinct from declaration-of-X.

   *Root cause CORRECTED during the upstreaming pass* (the original "shadow lives
   in a class template specialization" attribution was wrong — that shape mangles
   fine at the unfixed base). Two actual defects in the name-only mangling:
   - **Operator-named shadows crash**: `mangleUnqualifiedName` casts the decl to
     `FunctionDecl` to disambiguate the operator name, and a `UsingShadowDecl` is
     not one. A plain class reproduces (`struct S : private OB { using
     OB::operator*; };` + proxy NTTP). StatusOr crashed because it re-exports
     `operator*`/`operator->` via `using StatusOr::OperatorBase::...`.
   - **Overload-set collision**: one using-declarator over N same-named overloads
     introduces N shadows whose proxies all mangled identically (name-only) —
     `definition with same mangled name '_Z5probeIMaN1SIiE5valueE$EEiv'` at best,
     a TC-0004-style silent linkonce_odr fold at worst. The target-based mangling
     fixes this too (each shadow mangles via its own target overload).
2. **`is_rvalue_reference_qualified(underlying_entity_of(proxy))` misreports
   `false`** for `&&`-qualified members reached through a proxy on an
   instantiated class template (all four `StatusOr<int>::value()` overloads
   report `false`; a plain non-template fixture reports correctly). NOT fixed in
   the toolchain (root cause in how the underlying decl is resolved during
   instantiation); **worked around in the binder**: `reflect_bind_proxy` gates on
   "does a `reflect_method_binder` partial specialization exist for the exact
   function type" (`sizeof` on the undefined primary is a substitution failure),
   which filters volatile/`&&` shapes without trusting the decl predicates.

   *Re-probed during the TC-0004 root-cause (still OPEN, sharper picture now):*
   this is NOT the TC-0004 mangling collapse (the TC-0004 fix is scoped to
   function-template reflections and does not touch this path; the misreport
   persists after it). A minimal shape — non-template `D : private OB<int>`
   with `using OB<int>::value;`, four cv/ref-qualified overloads — answers ALL
   qualifier predicates correctly at every nesting depth, including through
   NTTP dispatch. Against the real `absl::StatusOr<int>`, the same probe
   reports `is_const`/`is_lvalue_reference_qualified`/
   `is_rvalue_reference_qualified` ALL false for ALL four `value()` proxies'
   underlyings, while `is_function` is true — i.e. the underlying entity IS a
   function but its qualifiers are invisible. Two minimal shapes were ruled
   out: `using` in a plain class over an instantiated base (`D : private
   OB<int>`) AND `using` inside a class template over a dependent base
   (`template <class T> struct DT : private OB<T> { using OB<T>::value; }`,
   probed via `DT<int>`) both answer correctly. Whatever StatusOr adds beyond
   that — its multi-level internal_statusor base/using chain is the prime
   suspect — is still unminimized. Probe scripts from this session:
   the StatusOr probe compiles against `corpus/libs/abseil` +
   `build/abseil-install/lib/libabsl_merged.a` (+ `-framework CoreFoundation`).

## Binder-side hardening (independent of the toolchain fix)

`nb_reflect.h` orders `is_using_proxy(fn)` guards before any kind-predicate in every
`members_of` loop (the constructor pass comment cites this finding), so the binder
does not rely on the patched toolchain.

Rebuild: `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.
