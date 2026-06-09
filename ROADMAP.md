# Roadmap — C++26 reflection → Python bindings prove-out

The original goal: a reflection-driven library where `nb::reflect_<^^some_namespace>(m)`
generates a working, importable nanobind module for a C++ namespace, **incrementally
covering more and more of C++** and scaling to a real codebase. This file records what's
done and the intended next steps, in priority order, with effort and approach notes so the
investigation can resume cleanly.

Status legend: ✅ done · 🔜 next (small/high-value) · 🧭 medium · 🧱 hard/research ·
⚙️ cross-cutting/productionization.

## Where we are

The binder (`nanobind/include/nanobind/nb_reflect.h`, branch `mk-reflect`) walks a namespace
via reflection and emits ordinary nanobind calls through splices. The umbrella repo builds
its own clang-p2996 toolchain (`./toolchain/`) and is self-contained; 31 reflection tests
pass. User-facing reference: `nanobind/docs/reflection.rst`.

### ✅ Done (one binder commit each)

- Classes: ctors, public data members (`def_rw`/`def_ro`), static data, methods, static
  methods, overloads.
- Function-type qualifier matching: `const`, `noexcept`, lvalue-ref (`&`); graceful skip of
  `volatile` / `&&` / C-variadic.
- Operators → Python dunders; conversion ops → `__bool__`/`__int__`/`__float__`.
- Enums.
- Inheritance: single base (real Python base, transitive + idempotent); multiple bases via
  member **flattening** (diamonds handled without double-binding).
- Virtual functions (Python overrides C++): two-tier — in-language trampoline **hook** +
  **codegen fallback** (`nb_reflect_codegen.h`) that emits trampoline source.
- Annotation control (`nb_reflect_annotations.h`): `skip`, `rename`, `doc`,
  `return_policy` (rv_policy), `keep_alive`.
- Self-contained toolchain build from the `llvm-project/` submodule.

## Next steps

### ✅ 1. Keyword-argument names (+ default arguments — blocked by the standard)
- **What:** emit `nb::arg("name")` per parameter so Python callers can use keywords. **Done**
  for methods, static methods, free functions, and constructors.
- **Why:** the single biggest ergonomics gap for real APIs; previously all args were
  positional and unnamed.
- **How it landed:** P3096 parameter reflection — `param_name<fn, I>()` reads
  `identifier_of` (static-stored via `define_static_string`); `with_arg_call_extras<fn>`
  prepends `nb::arg(...)` to the existing `with_call_extras` stream (and the ctor binder
  does the same on `init<...>()`). Only emitted when a function has ≥1 named parameter;
  unnamed params become `nb::arg()` to satisfy nanobind's all-or-nothing count. No new
  lambda signature names a spliced type, so the mangler-crash rule is untouched. Tested by
  `test30_keyword_arguments`.
- **Default *values*: not done — and not doable in-language.** This is a **gap in the C++26
  standard, not an implementation gap**: P3096 exposes only `has_default_argument` (a
  `bool`) with no `default_argument_of`/value accessor — a default argument is an arbitrary
  expression evaluated in the *caller's* context, not a reflectable entity. nanobind's
  `nb::arg("x") = value` needs the value, so generic default binding is impossible without a
  new WG21 facility. Left here as blocked-by-standard.

### ✅ 2. Free operators → reversed dunders
- **What:** namespace-scope binary `operator@(A,B)` → attach `__add__`/`__radd__` etc. to the
  relevant class. **Done.** (Previously free operators were skipped — they have no identifier.)
- **Why:** common for value types that define symmetric operators as free functions, and
  required for scalar-on-the-left expressions like `2.0 * vec`.
- **How it landed:** `bind_free_operators<T>(cls)` runs during `reflect_class<T>` (so the class_
  object is in hand — no re-registration). It scans `parent_of(^^T)` for binary operator
  functions and binds the forward dunder when T is the left operand and the reversed dunder
  (`operator_reversed_dunder`: `__r*__` for arithmetic, swapped name for comparisons) when T is
  the right operand (skipping the redundant reversed bind for symmetric same-type operators). The
  forwarding lambdas decompose the free-function type to `Ret(P0,P1)` so their signatures use real
  template params (mangler-crash rule). Tested by `test32_free_operators`, incl. `2.0 * vec`.

### ✅ 3. Class/enum docstrings
- **What:** honor `[[=r::doc{...}]]` on classes and enums. **Done.** The annotation follows the
  `struct`/`enum class` keyword, e.g. `struct [[=r::doc{"..."}]] Widget { ... };`.
- **Why:** completes the annotation vocabulary already shipped for functions/members.
- **How it landed:** a `with_doc_extra<R>(f)` helper calls the construction continuation with the
  `entity_doc<R>()` string as a trailing `const char*` extra (or nothing) and returns its result,
  so `reflect_class`'s 4-way `class_<...>` branch and `reflect_enum`'s `enum_<E>` just append
  `doc...` — no branch multiplication. nanobind treats a bare `const char*` extra as the type's
  docstring. Tested by `test31_class_enum_docstrings`.

### 🧭 4. Properties from getter/setter pairs
- **What:** recognize `getX()/setX(v)` (or `x()/x(v)`) conventions and bind a Python
  `@property` (`def_prop_rw`) instead of two methods.
- **Why:** idiomatic Python for encapsulated classes that hide fields behind accessors.
- **Approach:** annotation-driven first (`[[=r::property]]` or `[[=r::property{"name"}]]`),
  since name-convention sniffing is heuristic and error-prone. Then optionally a
  convention pass behind a flag.
- **Effort:** medium.

### ✅ 5. STL / type-caster coverage
- **What:** ensure the right `<nanobind/stl/*.h>` casters are present for the std types in bound
  signatures (string, vector, map, optional, unique_ptr, shared_ptr, ...). **Done — both routes.**
- **Why:** previously the *test* included them by hand; a real generated module must pull the
  correct casters or binding a `std::vector<int>` member fails to compile.
- **How it landed:** a shared reflection core in `nb_reflect.h` (`stl_caster_header`,
  `collect_stl_types`/`required_stl_types`, recursing into value template args while skipping
  policy args like allocator/comparator via `is_stl_policy`) detects the std types used and maps
  each to its caster header. **Codegen route:** `emit_trampolines` now also emits the required
  `#include <nanobind/stl/...>` lines (truly automatic — a generated module needs no hand-listed
  casters; proven by `test_reflect_codegen` which drops its manual includes). **Header-only route:**
  `#include` can't be emitted from template code, a *standard* constraint, so `reflect_`
  `static_assert`s (P2741 message) naming the exact missing header (`check_stl_casters`); `is_base_caster_v`
  distinguishes a missing stl caster from a real binding, run only on reflection-identified std types.
  `nb::detail::required_stl_headers<^^ns>()` exposes the list. Only common std types recognized;
  types used solely by an external base outside the reflected set are not detected.
- **Effort:** medium; design-heavy (delivered).

### 🧱 6. Templates (class & function)
- **What:** bind specific instantiations of templated classes/functions.
- **Why:** real codebases are full of templates; currently skipped (`!is_template`).
- **Approach:** you cannot bind a template, only instantiations — so this is inherently
  **opt-in via an explicit instantiation list**, e.g. an annotation
  `[[=r::instantiate<Foo<int>, Foo<double>>]]` or a registration list passed to `reflect_`.
  Use `substitute`/`reflect_class` per instantiation; Python names need disambiguation
  (`Foo_int`, `Foo_double`). Pairs naturally with the annotation layer (#3 above pattern).
- **Effort:** hard; needs a naming scheme and a registration surface.

### 🧱 7. Trampoline generation hardening
- **What:** generate trampolines for `final`/ref-qualified virtuals (currently skipped) and
  validate virtual/diamond **base** layouts (untested).
- **Why:** completeness of the virtual-override feature on gnarly hierarchies.
- **Effort:** medium–hard; mostly edge-case handling in `emit_trampolines`.

### 🧱 8. Per-argument ownership transfer
- **What:** an annotation for "C++ takes ownership of this argument" (sink parameters),
  beyond return-value `rv_policy` + `keep_alive`.
- **Why:** the user said rv_policy + keep_alive suffice today; keep this on the shelf until a
  real API needs it.
- **Effort:** medium; nanobind support for arg-ownership is limited — research first.

## ⚙️ Cross-cutting / productionization

- **CMake helper for end users:** a `nanobind_reflect_module(target NAMESPACE ^^ns ...)`
  function that wires the generator (for trampolines) + the module build in one call, so a
  consumer doesn't hand-write the two-stage dance.
- **Full codegen path (the original "emit source to file" idea):** generalize
  `nb_reflect_codegen.h` from trampolines-only to emitting the *entire* binding TU as text.
  Payoff: the generated `.so` could be compiled by a **stock** compiler (only the generator
  needs p2996), and bindings become human-readable/debuggable. Cost: a C++ type-name speller
  (or the splice-by-index trick used for trampolines) and the two-stage build. Revisit once
  #5 (caster includes) is in, since they share the codegen machinery.
- **Compile-time cost:** `<meta>` + `template for` over large namespaces is slow; measure and,
  if needed, shard or cache. Relevant before pointing at a big real namespace.
- **Name-collision policy:** define behavior when two entities map to the same Python name
  (across flattened bases, overloaded operators, renamed members). Today nanobind overloads
  or last-wins; make it explicit/diagnosable.
- **Submodule portability:** the `llvm-project` submodule URL is the local checkout (its
  pinned commit isn't pushed). If this prove-out ever needs to move machines, push that
  commit to `Cfretz244/llvm-project` and repoint the URL.

## Suggested order

`#1 (kwarg names)` ✅ → `#3 (class docstrings)` ✅ → `#2 (free operators)` ✅ →
`#5 (caster includes)` ✅ → `#4 (properties)` → then the hard ones (`#6 templates`,
`#7 trampoline hardening`) as real-codebase needs dictate. The CMake helper and full-codegen
generalization are worth doing once a second real consumer appears.
