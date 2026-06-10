## Summary

This is a gap in #286's fix (PR #287), found in the field a few hours after preparing it — same silent-fold failure mode, new trigger.

The NTTP discriminator for overloaded function-template reflections appends an ODR hash of the template parameter list plus the declaration pattern via `ODRHash::AddFunctionDecl(pattern, /*SkipBody=*/true)`. But `AddFunctionDecl` **silently no-ops for any declaration in "specialization context"** — its first step walks the decl contexts and returns on finding a `ClassTemplateSpecializationDecl` (`ODRHash.cpp`, "Skip functions that are specializations or in specialization context"). A member template of an *instantiated class template* — the common `members_of` shape — therefore contributes only its template HEAD to the hash.

#287's own field shape escaped by luck: absl's `operator[]` siblings have *different* heads (the SFINAE-false pack twin). Same-named siblings with **identical** heads still mangle identically:

```
error: definition with same mangled name
'_Z5probeIMtN2tl8expectedIiNSt..._5valueIEE$1393736719$EEiv' as another definition
```

— or, in a TU that never collides explicitly, the familiar silent linkonce_odr fold where one dispatcher body serves every sibling's call site. The AST-level specializations are correct and distinct; no diagnostic.

## Field evidence

`tl::expected<T, E>` (TartanLlama/expected v1.3.1) declares four `value()` member templates sharing one head and differing only in cv/ref qualifiers + return type:

```cpp
template <class U = T, detail::enable_if_t<!std::is_void<U>::value>* = nullptr>
TL_EXPECTED_11_CONSTEXPR const U &value() const & { ... }
// + & / const&& / && siblings, identical heads
```

A reflection-driven binding generator dispatches each through a `reflect_bind_member_template<T, tmpl>` NTTP; all four reflections mangled identically, CodeGen folded the four dispatch bodies into one (an rvalue-ref-qualified sibling's, which binds nothing), and `value()` silently never appeared on the bound class. Caught by a differential test suite, not by any diagnostic.

## Reproducer

Self-contained, `-std=c++26 -freflection-latest`, needs `-c` (codegen):

```cpp
#include <experimental/meta>

namespace meta = std::meta;

template <class T> struct trait { static constexpr bool value = true; };

// Same-headed qualifier siblings as members of a class template -- their
// reflections come from members_of over the SPECIALIZATION Exp<int>.
template <class T> struct Exp {
  template <class U = T, bool = trait<U>::value>
  const U &value() const & { return v; }
  template <class U = T, bool = trait<U>::value>
  U &value() & { return v; }
  T v;
};

template <meta::info R> int probe() { return 1; }

int main() {
  int (*addrs[8])() = {};
  int n = 0;
  template for (constexpr auto m : std::define_static_array(
      meta::members_of(^^Exp<int>, meta::access_context::unchecked()))) {
    if constexpr (meta::is_function_template(m)) {
      addrs[n++] = &probe<m>;
    }
  }
  return n == 2 && addrs[0] != addrs[1] ? 0 : 1;
}
```

Build matrix:

- `clang++ -c repro.cpp` (with #287 applied) — `error: definition with same mangled name ... as another definition` (the bug)
- `clang++ -fsyntax-only` — clean (front-end fine; it is the mangler)
- the same two signatures at namespace scope — clean (no specialization context: `AddFunctionDecl` hashes them)

## Expected

Compiles and runs to exit 0: two distinct `probe<m>` instantiations.

## Actual (at `837da39eb88c` + #287)

The duplicate-mangled-name error above; or, where the TU permits, a silent fold.

## Suggested fix (PR follows shortly)

In the same hash block, additionally hash what `AddFunctionDecl` skips in specialization context: the pattern's function type via `ODRHash::AddQualType` (return type, parameter types, cv-quals — the ODR *hash* handles the dependent pattern types that a structural *mangling* of the type cannot, the original #286 constraint), plus the ref-qualifier, which even `VisitFunctionProtoType` omits (two siblings can differ in nothing else). The regression test covers the four-sibling `value()` shape and a ref-qualifier-only pair, asserted pairwise distinct at runtime.

Note: the PR stacks on #287 and the deduction-guide PR (it amends the same hash block).
