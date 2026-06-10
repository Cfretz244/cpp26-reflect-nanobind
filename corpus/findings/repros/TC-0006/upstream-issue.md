## Summary

`std::meta::can_substitute(tmpl, args)` — the SFINAE-probe metafunction that is supposed to answer `false` for failing substitutions — crashes the frontend when substituting the (individually valid) arguments into the *declaration* forms an invalid type inside a template-id: e.g. a reference to void, the canonical `enable_if`-era immediate-context failure that every SFINAE-heavy library leans on. `CheckTemplateArgumentList` succeeds (void is a perfectly valid argument for `class OT`); the failure only surfaces while instantiating the declaration, and each template kind mishandles it differently:

1. **Function templates — SIGSEGV.** `Sema::InstantiateFunctionDeclaration` runs in a SFINAE context and returns null; `MetaActionsImpl::Substitute` dereferences it (`Spec->getType()`).
2. **Alias templates — diagnostic leak.** `CheckTemplateIdType` emits a hard `error: cannot form a reference to 'void'` out of a `can_substitute` probe, and the evaluation is non-constant instead of returning `false`.
3. **Variable templates — assertion failure.** `Assertion failed: (Spec && "substitution failed after validating arguments?")` in `ExprConstantMeta.cpp` (a null deref in a no-asserts build).
4. Concept paths happen to behave (the concept-id is valid and evaluates `false`) but share the unguarded shape.

## Field evidence

`tl::expected<void, E>` (TartanLlama/expected v1.3.1) declares

```cpp
template <class OT = T, class OE = E>
detail::enable_if_t<detail::is_swappable<OT>::value && ...> swap(expected&);
```

whose **defaulted** parameter picks up the enclosing specialization's `void` argument; `detail::is_swappable<void>` forms `declval<OT&>` → `void&` inside a template-id. A reflection-driven binding generator probes every member template with `can_substitute(mem, {})` to decide default-instantiability, so *merely reflecting over `expected<void, E>`'s members* crashes the compiler (exit 139). Plain C++ use of `expected<void, E>` — including `is_swappable<void>::value == false` — compiles fine; only the reflection probe dies.

## Reproducer

Self-contained, `-std=c++26 -freflection-latest`, `-fsyntax-only` suffices:

```cpp
#include <experimental/meta>
#include <vector>

namespace meta = std::meta;

template <class T> struct trait { using type = void; };

// Substituting OT=void forms trait<void&>: invalid type in the immediate
// context => substitution failure, NOT a crash.
template <class OT> typename trait<OT&>::type f() {}

// The field shape: a member template whose DEFAULTED parameter picks up the
// enclosing specialization's void argument (tl::expected<void,E>::swap).
template <class T> struct Exp {
  template <class OT = T>
  typename trait<OT&>::type swap(Exp&) {}
};

consteval bool probe_member_templates(meta::info cls) {
  for (auto mem : meta::members_of(cls, meta::access_context::unchecked()))
    if (meta::is_function_template(mem))
      (void)meta::can_substitute(mem, std::vector<meta::info>{});
  return true;
}

// Control: non-void substitution works.
static_assert(meta::can_substitute(^^f, std::vector<meta::info>{^^int}));

// Crash #1: explicit void argument on a free function template.
static_assert(!meta::can_substitute(^^f, std::vector<meta::info>{^^void}));

// Crash #2: the defaulted-parameter member-template form (the binder's walk).
static_assert(probe_member_templates(^^Exp<void>));

int main() {}
```

Sibling shapes for the other template kinds (each independently misbehaves at base):

```cpp
template <class T> using Ref = typename trait<T&>::type;    // alias:  hard error leaks, non-constant
template <class T> typename trait<T&>::type* vt = nullptr;  // var:    assertion failure
// !can_substitute(^^Ref, {^^void}) / !can_substitute(^^vt, {^^void}) should both just be true
```

A failure while forming the *argument list* (e.g. an invalid default template argument) is already handled correctly — only failure during *declaration* substitution crashes.

## Expected

Compiles: `can_substitute` answers `false` for the void substitutions (and `substitute` is a non-constant evaluation with a note), exactly as it already does for argument-list-level failures and undeduced placeholders.

## Actual (at `837da39eb88c`)

```
Stack dump:
 ...
 #5 (anonymous namespace)::MetaActionsImpl::Substitute(clang::FunctionTemplateDecl*, llvm::ArrayRef<clang::TemplateArgument>, clang::SourceLocation)
 ...
clang++: error: clang frontend command failed with exit code 139
```

## Suggested fix (PR follows shortly)

The `MetaActions::Substitute` overloads gain a `SuppressDiagnostics` flag (threaded from the metafunction's `NoDiagnose`, matching the existing `CheckTemplateArgumentList` contract), wrap their Sema calls in `Sema::SuppressDiagnosticsRAII`, and return null on failure instead of crashing; the `substitute` metafunction maps null to `ElideDiagnosis()` under `NoDiagnose` (`can_substitute` → `false`) or to a new `metafn_substitution_failed` note (`substitute` → non-constant), the same shape as the existing `metafn_undeduced_placeholder` path. Regression tests included with the PR.
