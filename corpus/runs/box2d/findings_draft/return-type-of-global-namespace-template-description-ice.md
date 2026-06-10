dedup_key: return-type-of-description-of-unhandled-template-kind-ice

# `nb::reflect_` of any GLOBAL-namespace class ICEs: `DescriptionOf` "unhandled template kind"

## Layer
TOOLCHAIN (root cause) + BINDER (contributing call site).

## Smallest trigger (4 lines, no library needed)

```cpp
// crash.cpp
#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
struct V { float x, y; float len() const { return x*x+y*y; } };  // GLOBAL namespace
NB_MODULE(d_ext, m){ nb::reflect_<^^V>(m); }
```

Compile (the corpus binding flags):

```
toolchain/bin/clang++ -std=c++26 -freflection-latest -fentity-proxy-reflection -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -I <python-include> -I nanobind/include \
  -fsyntax-only crash.cpp
```

## First diagnostics

```
UNREACHABLE executed at .../clang/lib/AST/ExprConstantMeta.cpp:1772!
  #8  clang::DescriptionOf(clang::APValue, bool)
  #9  clang::return_type_of(clang::APValue&, clang::ASTContext&, clang::MetaActions&, ...)
  #10 clang::Metafunction::evaluate(...)
  ...
clang frontend command failed with exit code 134
unhandled template kind
```

`ExprConstantMeta.cpp:1772` is the `llvm_unreachable("unhandled template kind")` at the end of
`DescriptionOf`'s `case ReflectionKind::Template:` switch (handles only
Function/Class/TypeAlias/Var template + Concept). `return_type_of` (same file, ~line 6514)
reaches it on its diagnostic path: when handed a `ReflectionKind::Template` reflection it does
`Diagnoser(...) << DescriptionOf(RV)`, and `DescriptionOf` crashes for a `TemplateDecl` of an
unhandled kind (a `BuiltinTemplateDecl` / deduction-guide-class Template reflection that the
global-namespace walk surfaces during the binder's `template for` instantiation context).

## Why it fires

- The crash is specific to a class whose enclosing scope is the **global namespace**
  (`parent_of(^^V) == ` the TranslationUnit). A class in a *named* namespace does NOT crash;
  a global-scope **enum** does NOT crash (only classes run the free-operator scan).
- The binder call site is `bind_free_operators<T>` (only invoked from `reflect_class`, hence
  classes-only, enums exempt). It scans `parent_of(^^T)` for free operators; on that path
  `return_type_of` is evaluated (line 291 `returns_raw_class_pointer` via `effective_rv_policy`,
  and/or line 1133 `fn_mentions_excluded`, which guards only `!is_constructor && !is_destructor`,
  NOT `!is_template`). When the scanned global-scope member is a Template reflection of an
  unhandled `DescriptionOf` kind, the toolchain aborts.

Isolation notes for triage: enumerating `members_of(parent_of(^^V))` in a standalone program
and calling `return_type_of` on every member reproduces a *clean* diagnostic for the visible
global templates (libc++'s `__make_integer_seq` / `__type_pack_element` / `__builtin_common_type`
reflect as `ReflectionKind::Type`, not Template, so they hit the graceful
"cannot introspect a non-function type" path). The crashing reflection appears only inside the
binder's full instantiation context, so the exact offending entity should be pinned with a
breakpoint on `ExprConstantMeta.cpp:1772` (it is a `TemplateDecl` whose `isa<>` matches none of
FunctionTemplate/ClassTemplate/TypeAliasTemplate/VarTemplate/Concept).

## Two-layer fix sketch (for triage; NOT applied here per corpus rules)

- TOOLCHAIN (root, upstreamable): make `DescriptionOf`'s `ReflectionKind::Template` switch
  exhaustive — a trailing `return "a template";` instead of `llvm_unreachable`, so any
  metafunction's diagnostic path degrades to a clean error rather than an ICE. (Category B/C on
  the campaign tracking issue: a kind-blind metafunction diagnostic helper.)
- BINDER (defensive): guard the `return_type_of` calls on the global-namespace scan with
  `is_function(fn) && !is_template(fn)` (line 1133 `fn_mentions_excluded` already skips
  ctors/dtors but not templates; `returns_raw_class_pointer` at line 289 has no guard at all).

## Impact on this run

Blocks the entire box2d run at Gate 4. Every box2d public type (b2Vec2/b2World/b2Body/b2Fixture/
b2BodyDef/b2PolygonShape/...) lives in the **global namespace** and is a **class**, so each one
trips this on its own — there is no smaller surviving subset. Confirmed on the real header:
`nb::reflect_<^^b2Vec2>(m)` with `<box2d/b2_math.h>` crashes identically.
