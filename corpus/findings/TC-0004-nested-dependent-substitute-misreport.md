# TC-0004 — same-named function-template reflections as NTTPs mangle identically; codegen silently folds the dispatch instantiations

(Originally recorded as "substitute() results misreport predicates under nested-dependent
instantiation" — minimization showed the predicates never misreported; see Root cause.)

- **Status:** FIXED locally (llvm-project @ `b329d544cb20`,
  `clang/lib/AST/ItaniumMangle.cpp`); **upstreamed**: issue
  [bloomberg/clang-p2996#286](https://github.com/bloomberg/clang-p2996/issues/286) + PR
  [#287](https://github.com/bloomberg/clang-p2996/pull/287) (see
  `repros/TC-0004/UPSTREAM.md`). The binder workaround has been REMOVED (substitution
  happens inline in `reflect_bind_member_template` again).
- **Found via:** Wave-4 container binding. `absl::flat_hash_map<int,std::string>` bound with
  the full member-template surface (`contains`/`find`/`at`/`count`/`erase`) **except**
  `operator[]` — silently absent, no diagnostic.
- **Triage:** `toolchain`. dedup_key: `nested-dependent-substitute-misreport`.
- **Standalone repro:** `corpus/findings/repros/TC-0004/repro.cpp` (no nanobind; compile
  command in its header). Regression tests:
  `llvm-project/libcxx/test/std/experimental/reflection/substitute-nested-dependent.pass.cpp`
  and the SFINAE-false pack-sibling `operator[]` on `HetMap` in
  `nanobind/tests/test_reflect.cpp` (exercised by `test37`).

## Symptom (as observed in the binder)

A member function template's default instantiation, obtained via
`std::meta::substitute(tmpl, {})` two dependent levels deep (a function template
instantiated from a `template for` body inside another dependent function template),
appeared to answer kind/qualifier predicates (`is_operator_function`, `is_volatile`,
`is_rvalue_reference_qualified`) incorrectly — silently. The same calls evaluated
correctly at non-dependent scope, one dependent level down, and when the spec was
computed at a shallower level and passed down as an NTTP. The failure required a
SFINAE-false pack-sibling overload of the same operator template to be present (absl
raw_hash_map's lifetimebound `operator[]` pair).

## Root cause (established by the standalone repro)

The predicates were never wrong — **the wrong function body ran**:

- `ItaniumMangle.cpp`'s `mangleReflection`, `ReflectionKind::Template` case, mangled a
  reflected template as its *name only*. Class/variable/alias templates cannot be
  overloaded, so that is sufficient for them — but **function templates overload**, and
  two same-named sibling `operator[]` templates produced byte-identical manglings.
- A dispatcher `level2<T, tmpl>` instantiated for each sibling therefore got ONE mangled
  name for its two specializations. The AST-level specializations were correct and
  distinct (verified by `-ast-dump`: each body had the right `can_substitute` constant);
  CodeGen then silently folded the linkonce_odr definitions by mangled name, and a single
  body — the SFINAE-false sibling's, which binds nothing — served both call sites.
- This explains every bisect observation: no sibling → single instantiation → no
  collision; spec-passed-as-NTTP → spec reflections are *declarations* and mangle with
  their full signature → distinct; "one dependent level works" → that shape had no
  reflection-NTTP dispatcher at all. Nesting depth was never the variable.

## Fix (llvm-project submodule)

`ReflectionKind::Template` mangling now appends, for `FunctionTemplateDecl`s only, an
**ODR hash** of the template parameter list + the templated declaration pattern
(`ODRHash::AddTemplateParameterList` + `AddFunctionDecl`), bracketed in `$`. The hash is
cross-TU-stable by design (it is how modules compare decls between TUs), so linkonce_odr
merging of genuinely-identical specializations still works. A structural mangling of the
pattern's function type was tried first and abandoned: real-world dependent pattern types
(absl's lifetimebound SFINAE, `noexcept(...)` referencing parameters) embed
parameter-referencing expressions that `mangleFunctionParam` cannot encode outside a
function-declaration context (assertion failure across the abseil corpus).

## Binder aftermath

The workaround (substitute hoisted to the dispatch loop, spec passed as a frozen NTTP) has
been **removed**: `reflect_bind_member_template<T, tmpl>` substitutes inline again, the
exact previously-broken shape, now covered by the pack-sibling regression shape in the
binder test suite. The "binder-spec completeness" qualifier gate
(`requires { sizeof(reflect_method_binder<T, fn, FnType>); }`) is NOT a TC-0004 artifact
and stays: it is the binder's volatile/&&-shape filter, and decl predicates remain
untrustworthy for *proxy underlyings* from instantiated class templates (TC-0003
addendum, still open — see that finding for fresh evidence).

## Upstreaming

Draft issue text for bloomberg/clang-p2996 in `repros/TC-0004/UPSTREAM.md` (not yet
filed). The repro needs no `-fentity-proxy-reflection` — plain `-freflection-latest`.
