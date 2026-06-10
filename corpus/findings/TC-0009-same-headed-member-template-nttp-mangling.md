# TC-0009 — same-headed member-template reflections of a specialization mangle identically as NTTPs (value() silently unbound)

- **Status:** FIXED (toolchain) — root-caused and fixed in the pinned
  llvm-project (`clang/lib/AST/ItaniumMangle.cpp`, `mangleReflection`,
  `ReflectionKind::Template`); regression test
  `llvm-project/libcxx/test/std/experimental/reflection/fn-template-nttp-mangling-spec-context.pass.cpp`;
  prepared for upstream alongside TC-0006/0007/0008
- **Track:** toolchain (clang-p2996, pinned by this repo), Itanium mangler —
  a GAP in the TC-0004 fix (same hash block), different trigger: TC-0004
  discriminated same-named siblings with *different* template heads; this is
  same-named siblings with *identical* heads in specialization context
- **Found via:** `corpus/runs/expected` (TartanLlama/expected v1.3.1) — on the
  run's FIRST execution after TC-0008 + BINDER-0012 landed, the differential
  suite failed with `AttributeError: ... object has no attribute 'value'`:
  `expected<T,E>::value()` had silently never bound. This is exactly the
  silent-fold failure mode TC-0004 warned about, through a hole in its fix.
- **Repro:** `corpus/findings/repros/TC-0009/repro.cpp` (standalone; needs
  `-c`, `-fsyntax-only` is clean — it is the mangler; `-DFREE_CONTROL` shows
  the namespace-scope control compiling clean).

## Symptom

The TC-0004 NTTP discriminator appends an ODR hash of the template head plus
the declaration pattern (`ODRHash::AddFunctionDecl(pattern, SkipBody=true)`)
to a `ReflectionKind::Template` mangling. But `AddFunctionDecl` **silently
no-ops for any declaration in "specialization context"** — its first step
walks the decl contexts and `return`s on finding a
`ClassTemplateSpecializationDecl` (ODRHash.cpp, "Skip functions that are
specializations or in specialization context"). A member template of an
instantiated class template — the common `members_of` shape — therefore
contributes only its template HEAD to the hash.

tl::expected<T,E>'s four `value()` member templates share one head:

```cpp
template <class U = T, detail::enable_if_t<!std::is_void<U>::value>* = nullptr>
TL_EXPECTED_11_CONSTEXPR const U &value() const & { ... }
// + & / const&& / && siblings, identical heads
```

so all four reflections mangled identically
(`...5valueIEE$1393736719$...` — one shared `$hash$`):

```
error: definition with same mangled name
'_Z5probeIMtN2tl8expectedIiNSt..._5valueIEE$1393736719$EEiv' as another definition
```

— or, in a TU that never collides explicitly, a silent linkonce_odr fold where
one dispatch body serves all four call sites. TC-0004's analysis applies
verbatim: the AST-level specializations are correct and distinct; only a
runtime observation catches the fold.

## Field shape

The binder routes every default-instantiable member template through
`reflect_bind_member_template<T, tmpl>` — `tmpl` is a reflection NTTP. The
four `value()` instantiations folded into one body; whichever survived was a
rvalue-ref-qualified sibling whose binder gate binds nothing, so `value()`
just never appeared on the bound class. `swap()` (a single template, no
same-headed sibling) bound fine, which is what localized the bug.

## Fix

In `mangleReflection`'s function-template hash block, additionally hash what
`AddFunctionDecl` skips in specialization context: the pattern's function
type via `ODRHash::AddQualType` (return type, parameter types, cv-quals —
the ODR type hash handles the dependent pattern types that a structural
MANGLING of the type cannot, the original TC-0004 constraint) plus the
ref-qualifier (which even `VisitFunctionProtoType` omits — two siblings can
differ in nothing else).

## Layering in corpus/runs/expected

Unreachable until TC-0008 (mangler ICE earlier in the same TU) and
BINDER-0012 (deleted-ctor hard error) landed; surfaced as outcome D
(`D.value`, 8 differential failures) on the first post-fix run, and the run
reached E once this landed.
