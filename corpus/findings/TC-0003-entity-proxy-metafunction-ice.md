# TC-0003 — member-kind metafunctions ICE on entity proxies (using-shadow declarations)

- **Status:** FIXED locally (llvm-project submodule); candidate for upstreaming to
  bloomberg/clang-p2996.
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
| `identifier_of` (~2649) / `has_identifier` (~2745) | left as-is — empirically these already work on proxies (the unreachable arm is not hit; evidently unwrapped on another path) |
| `has_complete_definition` (~4789), `is_enumerable_type` (~4830) | left as-is (not demonstrated; type-oriented queries on a proxy are arguably invalid) |
| `reflect_invoke` (~7112) | left as-is (invoking through a proxy is meaningful-but-unimplemented; an ICE is still wrong but the right fix is unwrap-and-invoke, a bigger change) |

The four fixed predicates return `false` for `EntityProxy`, consistent with how e.g.
`is_literal_operator` (an `isReflectedDecl()`-style check) already degrades
gracefully. Inconsistent handling across metafunctions is the underlying issue; an
upstream issue should propose a uniform policy (answer the query on the proxy as if
asked of the shadow declaration, or transparently unwrap).

## Addendum: two more entity-proxy toolchain bugs (same flag, found minutes later)

1. **Itanium mangler ICE on proxy reflections as template arguments.**
   `reflect_bind_proxy<T, ^^proxy>` makes the proxy an NTTP; mangling it went
   through `mangleNameWithAbiTags(shadow-decl)`, which trips a `cast<>` assertion
   when the shadow lives in a class template specialization (`using
   OperatorBase<T>::value;` inside `absl::StatusOr<int>`), and the first fix
   attempt (`mangle(Target)`) hit the `mangle(GlobalDecl)` "unexpected kind of
   global decl" UNREACHABLE for non-function/variable targets. **Fixed locally**
   (`ItaniumMangle.cpp`, `ReflectionKind::EntityProxy` case): mangle the proxy's
   TARGET declaration kind-aware (functions/vars via `mangle`, enum constants as
   literals, types via `mangleCanonicalTypeName`, else the source name), keeping
   the `a` tag so proxy-of-X stays distinct from declaration-of-X.
2. **`is_rvalue_reference_qualified(underlying_entity_of(proxy))` misreports
   `false`** for `&&`-qualified members reached through a proxy on an
   instantiated class template (all four `StatusOr<int>::value()` overloads
   report `false`; a plain non-template fixture reports correctly). NOT fixed in
   the toolchain (root cause in how the underlying decl is resolved during
   instantiation); **worked around in the binder**: `reflect_bind_proxy` gates on
   "does a `reflect_method_binder` partial specialization exist for the exact
   function type" (`sizeof` on the undefined primary is a substitution failure),
   which filters volatile/`&&` shapes without trusting the decl predicates.

## Binder-side hardening (independent of the toolchain fix)

`nb_reflect.h` orders `is_using_proxy(fn)` guards before any kind-predicate in every
`members_of` loop (the constructor pass comment cites this finding), so the binder
does not rely on the patched toolchain.

Rebuild: `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.
