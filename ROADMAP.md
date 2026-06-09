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

### 🔜 1. Keyword-argument names + default arguments
- **What:** emit `nb::arg("name")` per parameter, and bind C++ default arguments so Python
  callers can use keywords/defaults.
- **Why:** the single biggest ergonomics gap for real APIs; today all args are positional
  and unnamed.
- **Approach:** P3096 parameter reflection — `parameters_of(fn)` gives `identifier_of` (name)
  and `has_default_argument`. Thread `nb::arg(...)` extras through the existing
  `with_call_extras` mechanism. Default *values*: read via the parameter reflection if
  extractable; otherwise generate a wrapper. Watch the **mangler-crash rule** — keep spliced
  types out of any new lambda signatures (these are extras, so low risk).
- **Effort:** small–medium.

### 🔜 2. Free operators → reversed dunders
- **What:** namespace-scope `operator@(A,B)` → attach `__add__`/`__radd__` etc. to the
  relevant class. Currently free operators are skipped (they have no identifier).
- **Why:** common for value types that define symmetric operators as free functions.
- **Approach:** in `reflect_dispatch`, detect `is_operator_function` free functions; map to a
  dunder on the type of the first (or second, for reversed) operand. Reuse `operator_dunder`.
- **Effort:** medium (deciding which class owns the binding; reversed-arg lambdas).

### 🔜 3. Class/enum docstrings
- **What:** honor `[[=r::doc{...}]]` on classes and enums (deferred earlier).
- **Why:** completes the annotation vocabulary already shipped for functions/members.
- **Approach:** thread an optional `const char*` doc into the 4-way `class_<...>` construction
  (and `enum_<...>`). The 4-way branch made this awkward; factor the doc-or-not choice so it
  doesn't multiply the branch count.
- **Effort:** small.

### 🧭 4. Properties from getter/setter pairs
- **What:** recognize `getX()/setX(v)` (or `x()/x(v)`) conventions and bind a Python
  `@property` (`def_prop_rw`) instead of two methods.
- **Why:** idiomatic Python for encapsulated classes that hide fields behind accessors.
- **Approach:** annotation-driven first (`[[=r::property]]` or `[[=r::property{"name"}]]`),
  since name-convention sniffing is heuristic and error-prone. Then optionally a
  convention pass behind a flag.
- **Effort:** medium.

### 🧭 5. STL / type-caster coverage (automatic includes)
- **What:** ensure the right `<nanobind/stl/*.h>` casters are present for the std types that
  appear in bound signatures (string, vector, map, optional, unique_ptr, shared_ptr, ...).
- **Why:** today the *test* includes them by hand; a real generated module must pull the
  correct casters or binding a `std::vector<int>` member fails to compile.
- **Approach:** walk all bound signatures, detect std container/wrapper types via reflection
  (`template_of`/`type_of`), and (a) in the header-only path, document the required includes,
  or (b) in the **codegen path**, emit the needed `#include`s automatically. This is the
  natural first real use of full source codegen beyond trampolines.
- **Effort:** medium; design-heavy (mapping C++ std types → caster headers).

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

`#1 (kwargs/defaults)` → `#3 (class docstrings)` → `#2 (free operators)` →
`#5 (caster includes)` → `#4 (properties)` → then the hard ones (`#6 templates`,
`#7 trampoline hardening`) as real-codebase needs dictate. The CMake helper and full-codegen
generalization are worth doing once a second real consumer appears.
