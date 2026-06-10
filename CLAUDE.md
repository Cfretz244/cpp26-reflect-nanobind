# CLAUDE.md — cpp26-reflect-nanobind (umbrella prove-out)

Guidance for Claude Code (claude.ai/code). This repo is the **umbrella** that captures an
ongoing, **this-laptop** investigation: using **C++26 static reflection (WG21 P2996)** to
**automatically generate Python bindings** with **nanobind**, plus standalone
reflection-driven demos. It is a prove-out, not a product. Paths below are deliberately
specific to this machine (a macOS / Apple Silicon laptop).

## Layout

This repo pins the two pieces as git submodules (GitHub fork URLs — see `.gitmodules`)
and adds demo programs:

```
cpp26-reflect-nanobind/
├── llvm-project/      submodule (url: Cfretz244/llvm-project fork) @ reflection-p2996
│                      The clang-p2996 fork (the compiler half). Its CLAUDE.md has
│                      build details. Built from here into ./toolchain/ (see
│                      "Build the toolchain"), making this repo self-contained.
├── nanobind/          submodule (url: Cfretz244/nanobind fork)   @ mk-reflect
│                      The reflection-driven binder (the active development surface).
│                      include/nanobind/nb_reflect*.h + tests/test_reflect*. See its
│                      CLAUDE.md and docs/reflection.rst.
├── examples/          standalone reflection demos from this prove-out:
│                      refl.cpp           – minimal: print a struct's fields
│                      serialize_poc.cpp  – a complete reflection-driven binary+JSON
│                                           serializer (no per-type code, no macros)
└── CLAUDE.md          (this file)
```

`git submodule status` shows the current pinned submodule commits (the captured state);
PROVE_OUT.md's status snapshot names the pins that carried each landed fix.

## The idea, in one paragraph

`nb::reflect_<^^my_namespace>(m)` (in `nanobind/include/nanobind/nb_reflect.h`) walks a C++
namespace with reflection and emits ordinary `nb::class_/.def` bindings via splices inside
`template for` loops — classes, inheritance (incl. multiple-base flattening), methods/
operators/enums, with per-entity control via `[[=...]]` annotations
(`nb_reflect_annotations.h`: skip / rename / doc / return-value policy / keep-alive). The
only thing that can't be done in-language — a virtual-override **trampoline** — has a
text-**codegen fallback** (`nb_reflect_codegen.h`) that emits trampoline source for a
two-stage build. Full feature list + limitations: `nanobind/docs/reflection.rst` and
`nanobind/CLAUDE.md`.

## Environment (this exact laptop)

- **Toolchain (built into this repo)**: `./toolchain/` — the clang-p2996 compiler
  (clang/clang++/lld + a libc++ providing `<meta>`/`<experimental/meta>`), **built from the
  `llvm-project/` submodule** into `./toolchain/` (via `./toolchain-build/`). This is what
  makes the repo self-contained: nothing here depends on the older `~/llvm-toolchain`
  anymore. Both `toolchain/` and `toolchain-build/` are git-ignored; rebuild instructions
  are below. (`~/llvm-toolchain` is the same compiler built earlier and may still exist, but
  is no longer required.) Native target AArch64.
- **Reflection flags**: `-std=c++26 -freflection-latest -fentity-proxy-reflection
  -stdlib=libc++`, plus (mandatory on macOS) `-isysroot "$(xcrun --show-sdk-path)"`. A
  from-source clang does not bake in the SDK path. `-freflection-latest` is the umbrella flag
  enabling P2996 + parameter reflection + expansion statements + annotations (P3394) + the
  rest — but NOT entity-proxy reflection (`using`-shadow enumeration), which the binder
  requires and must be passed explicitly.
- **Python**: Homebrew **`python3.12`** (`/opt/homebrew/bin/python3.12`). The system
  `/usr/bin/python3` lacks dev headers — do not use it.
- **Build state lives inside this repo** (both git-ignored, recreate from scratch any time):
  the venv at `.venv/`, the CMake build tree at `build/`. (Earlier work used `/tmp/nbbuild`,
  which is bound to the *old* `~/git/nanobind` source — do not reuse it from here.)
- Tools: `cmake`, `ninja`, `ccache` via Homebrew.

## First-time setup on a fresh checkout of THIS repo

```bash
cd ~/git/cpp26-reflect-nanobind
git submodule update --init --recursive
```
Both submodule URLs are GitHub forks: `nanobind` → `Cfretz244/nanobind` (branch `mk-reflect`),
`llvm-project` → `Cfretz244/llvm-project` (branch `reflection-p2996`; the bloomberg clang-p2996
fork plus a CLAUDE.md edit, its pinned commit `d4ae403` pushed there). So a fresh
`--init --recursive` clones cleanly on any machine — though the llvm-project history is ~4 GB,
and you still have to build `./toolchain/` from it (see "Build the toolchain").

## Build the toolchain (once; ~1–2h, ~3 GB installed + ~4 GB build tree) — verified

Builds the compiler from the `llvm-project/` submodule into `./toolchain/`. Bootstrapped
with Apple Clang; do **not** add `-DLLVM_USE_LINKER=lld` on a clean tree (Apple Clang can't
find an lld yet — hard error). Skip this whole section if `./toolchain/bin/clang++` exists.

```bash
cd ~/git/cpp26-reflect-nanobind
cmake -S llvm-project/llvm -B toolchain-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
  -DLLVM_TARGETS_TO_BUILD="AArch64" -DLLVM_CCACHE_BUILD=ON -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/toolchain"
ninja -C toolchain-build install     # the long part
```

### Local toolchain fixes baked into the `llvm-project` submodule (built in automatically)

The pinned `llvm-project` carries prove-out fixes on top of the bloomberg clang-p2996 fork;
a fresh build from the submodule includes them — no extra flags. If you already have a built
`./toolchain/`, re-run `ninja -C toolchain-build install` (or the targeted commands noted) to
pick them up:

- **Sema use-after-free under heavy reflection** (`clang/lib/Sema/SemaExpr.cpp`). Evaluating a
  large immediate (consteval) invocation — e.g. `nb::reflect_<^^nlohmann::json>` /
  `emit_trampolines` over `basic_json` — recursively pushes expression-evaluation contexts,
  reallocating `Sema::ExprEvalContexts` and dangling the `Rec` reference held across
  `HandleImmediateInvocations` / the tail of `PopExpressionEvaluationContext`. Manifested as a
  non-deterministic crash (garbage/ASCII pointer) once `-fconstexpr-steps` was raised enough to
  finish evaluating; **this is why raising the step budget appeared to "ICE" the compiler.** Fix
  re-acquires the record after reentrant evaluation; a follow-up audit hardened one more
  same-family site (`CheckLValueToRValueConversionOperand`) and added a standalone repro
  (`corpus/findings/repros/TC-0002/`) + regression test. Upstreamed as bloomberg/clang-p2996
  issue #288 / PR #289. (Rebuild: `ninja -C toolchain-build clang`
  then `ninja -C toolchain-build install-clang`.)
- **TC-0001 — missing Apple type-aware allocation operators in from-source libc++abi**
  (`libcxxabi/src/CMakeLists.txt` + `libcxxabi/lib/new-delete.exp`). Recent clang targeting macOS
  emits calls to `operator new[]/new/delete(..., std::__type_descriptor_t)`; those symbols live
  only in Apple's *system* libc++ and the vendor shim `libcxxabi/src/vendor/apple/shims.cpp`,
  which the normal build never compiles. A program using enough aggregate-with-method code (any
  nontrivial nlohmann/json use) then aborts at load: `dyld: Symbol not found:
  __ZnamSt19__type_descriptor_t`. (`-O2`/`-O3` only *sometimes* optimize the reference away — it
  reliably bites json.) The fix compiles the shim on Apple and exports its 10 symbols via
  `new-delete.exp`. (Rebuild: `ninja -C toolchain-build/runtimes/runtimes-bins cxxabi_shared &&
  ninja -C toolchain-build/runtimes/runtimes-bins install-cxxabi`; the `.exp` is not tracked as a
  link input, so `touch llvm-project/libcxxabi/src/vendor/apple/shims.cpp` first to force a
  relink.) The shim forwards each typed operator to the untyped one — the descriptor is an
  optional type-aware-allocation hint, exactly as Apple's own shim does.
- **TC-0003 — entity-proxy reflections ICE member metafunctions and the Itanium mangler**
  (`clang/lib/AST/ExprConstantMeta.cpp` + `clang/lib/AST/ItaniumMangle.cpp`). With
  `-fentity-proxy-reflection`, `members_of` enumerates using-shadow declarations as
  `EntityProxy` reflections, but six metafunctions (`is_constructor`, `is_destructor`,
  `is_special_member_function`, `is_static_member`, `is_enumerable_type`,
  `has_complete_definition`) treated that kind as `llvm_unreachable` — enumerating members
  and asking ordinary kind questions ICE'd. All six now answer `false` (a shadow decl is
  never itself one of these), matching the surrounding predicates. The mangler encoded a
  proxy NTTP by mangling the shadow's NAME: operator-named shadows (StatusOr's `using
  StatusOr::OperatorBase::operator*;`) crashed `mangleUnqualifiedName` outright, and an
  overload set behind one using-declarator mangled all its shadow proxies identically;
  it now mangles the proxy's TARGET decl kind-aware, keeping the `a` tag. Repros:
  `corpus/findings/repros/TC-0003/`; regression test
  `llvm-project/libcxx/test/std/experimental/reflection/entity-proxy-member-queries.pass.cpp`.
  Upstreamed as bloomberg/clang-p2996 issue #290 / PR #291. (Rebuild:
  `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.)
- **TC-0005 — function-type metafunctions blind to AttributedType sugar**
  (`clang/lib/AST/ExprConstantMeta.cpp`; promoted from TC-0003's addendum, which had
  mis-attributed it to entity proxies). `[[clang::lifetimebound]]` on a method (Abseil's
  `ABSL_ATTRIBUTE_LIFETIME_BOUND` on every accessor, e.g. all four
  `StatusOr<int>::value()` overloads) wraps its `FunctionProtoType` in `AttributedType`
  sugar; seven sugar-blind `dyn_cast<FunctionProtoType>` sites made
  `is_const`/`is_volatile`/`is_lvalue_reference_qualified`/`is_rvalue_reference_qualified`
  silently answer false and made `return_type_of`/`parameters_of`/`has_ellipsis_parameter`
  reject the function's type outright. All now desugar via `getAs<FunctionProtoType>`
  (as `is_noexcept` always did). Repro: `corpus/findings/repros/TC-0005/`; regression
  test `llvm-project/libcxx/test/.../attributed-function-type-queries.pass.cpp`.
  Upstreamed as bloomberg/clang-p2996 issue #292 / PR #293. (Rebuild:
  `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.)
- **TC-0004 — same-named function-template reflections as NTTPs mangled identically**
  (`clang/lib/AST/ItaniumMangle.cpp`, `mangleReflection`, `ReflectionKind::Template`). A
  reflected template mangled as its *name only*; function templates overload, so a dispatcher
  `f<T, tmpl>` instantiated for two same-named siblings (absl raw_hash_map's `operator[]` and
  its SFINAE-false lifetimebound pack sibling) got ONE mangled name — CodeGen silently folded
  the linkonce_odr definitions and a single body served both call sites. The AST-level
  specializations were correct, so `flat_hash_map`'s `operator[]` just dropped with no
  diagnostic (originally mis-read as predicate misreports; corrected in the finding). Fix
  appends an ODR hash of the template head + declaration pattern for `FunctionTemplateDecl`s
  (a structural mangling of the pattern type crashes on real-world dependent types —
  lifetimebound SFINAE / `noexcept(...)` referencing parameters). Standalone repro:
  `corpus/findings/repros/TC-0004/repro.cpp`; regression tests in
  `llvm-project/libcxx/test/std/experimental/reflection/substitute-nested-dependent.pass.cpp`
  and the binder's HetMap pack-sibling `operator[]`. The binder's dispatch-level substitution
  workaround has been removed. (Rebuild: `ninja -C toolchain-build clang &&
  ninja -C toolchain-build install-clang`.)
- **TC-0006 — `can_substitute`/`substitute` crashed when substitution forms an invalid type**
  (`clang/lib/Sema/SemaReflect.cpp`, `clang/lib/AST/ExprConstantMeta.cpp`,
  `clang/include/clang/AST/MetaActions.h`, `DiagnosticMetafnKinds.td`). Substituting valid
  arguments whose *declaration* substitution fails in the immediate context (reference to
  void inside a template-id — `tl::expected<void,E>::swap<OT=void>`, the classic `enable_if`
  shape) SIGSEGV'd (function templates: null `Spec` deref in `MetaActionsImpl::Substitute`),
  leaked a hard error (alias templates), or tripped an assert (variable templates). The
  `MetaActions::Substitute` overloads now take `SuppressDiagnostics`, return null on failure,
  and the metafunction maps null to `can_substitute == false` / a `metafn_substitution_failed`
  note. Repro: `corpus/findings/repros/TC-0006/`; regression tests
  `can-substitute-invalid-type-formation.pass.cpp` + `substitute.verify.cpp`. Upstreamed
  as bloomberg/clang-p2996 issue #294 / PR #295.
- **TC-0007 — type-completing metafunctions eagerly instantiated member bodies**
  (`clang/lib/Sema/SemaReflect.cpp`, `EnsureInstantiated`). Reflection-triggered completion
  used `TSK_ExplicitInstantiationDefinition` + `InstantiateClassTemplateSpecializationMembers`,
  wrong-rejecting specializations with lazily-ill-formed member bodies (the specialized-
  storage-base idiom, `tl::expected<void,E>`) — order-dependently (pre-instantiating the class
  dodged it). Now mirrors `RequireCompleteTypeImpl` (`TSK_ImplicitInstantiation`, declarations
  only, plus a member-class branch). Repro: `corpus/findings/repros/TC-0007/`; regression test
  `members-of-lazily-ill-formed-bodies.pass.cpp`. Upstreamed as bloomberg/clang-p2996
  issue #296 / PR #297.
- **TC-0008 — deduction-guide reflections ICE'd the Itanium mangler**
  (`clang/lib/AST/ItaniumMangle.cpp`, `mangleReflection`). `members_of` over a namespace
  enumerates guides; lifting the list into `define_static_array` mangles each reflection as a
  template argument, hitting `llvm_unreachable("Can't mangle a deduction guide name!")`
  (tl's `unexpected(E) -> unexpected<E>`; binding ANY tl class died at codegen). Guides now
  mangle as `"dg"` + deduced template + the TC-0004 ODR hash with `isImplicit()` + deduction-
  candidate kind folded in (implicit per-ctor guides are structurally identical to
  same-signature explicit ones). The binder ALSO strips guides pre-lift
  (`namespace_members_for_binding`), so it works on unpatched toolchains. Repro:
  `corpus/findings/repros/TC-0008/`; regression test
  `deduction-guide-reflection-mangling.pass.cpp`. Upstreamed as bloomberg/clang-p2996
  issue #298 / PR #299 (stacked on TC-0004's PR #287).
- **TC-0009 — same-headed member-template reflections of a specialization mangled identically**
  (`clang/lib/AST/ItaniumMangle.cpp`, the TC-0004 hash block). `ODRHash::AddFunctionDecl`
  silently no-ops in "specialization context", so `tl::expected<T,E>`'s four `value()` member
  templates (identical heads, differing only in cv/ref qualifiers + return type) hashed
  identically — codegen folded the binder's dispatch bodies and `value()` silently never bound
  (caught by the expected run's differential suite, not a diagnostic). The hash now also folds
  in the pattern's function type (`AddQualType`) + ref-qualifier. Repro:
  `corpus/findings/repros/TC-0009/`; regression test
  `fn-template-nttp-mangling-spec-context.pass.cpp`. Upstreamed as bloomberg/clang-p2996
  issue #300 / PR #301 (stacked on PR #299). (Rebuild for any of TC-0006..0009:
  `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.)

- **TC-0010 — reflections of a re-opened namespace compared unequal across redeclarations**
  (`clang/lib/AST/APValue.cpp`, `profileReflection`). `^^ns` wraps the namespace's first
  declaration; `parent_of(^^member)` wraps the block that declared the member; the
  `ReflectionKind::Namespace` case profiled the raw decl pointer (unlike Template, which
  canonicalizes), so `parent_of(x) == ^^ns` silently answered false for anything declared
  after the first block. Field shape: Eigen re-opens `Eigen::internal` everywhere, so the
  binder's namespace-level `nb::exclude_` never matched. Fix profiles the canonical decl
  (a NamespaceAliasDecl stays distinct from its target). Repro:
  `corpus/findings/repros/TC-0010/`; regression test
  `namespace-reflection-equality-reopened.pass.cpp`. Upstream draft prepared.
- **TC-0011 — namespace members_of derailed by out-of-line member definitions**
  (`clang/lib/AST/ExprConstantMeta.cpp`, `isReflectableDecl` + `findIterableMember`). A
  re-opened namespace block BEGINNING with an out-of-line class-member definition (Eigen's
  EulerAngles.h shape) made the walk yield the dependent definition PATTERN as a "member"
  (whose reflection crashes display/mangling: `isa<> used on a null pointer`) and then
  silently DROP every remaining member (69 of 125 Eigen members). Fix: such definitions are
  rejected (semantic ctx is a CXXRecordDecl, lexical differs) and stepping from one walks
  the LEXICAL chain. Repro: `corpus/findings/repros/TC-0011/`; regression test
  `namespace-members-out-of-line-defs.pass.cpp`. Upstreamed as issue #303 / PR #306
  (tracking issue #308).
- **TC-0012 — is_complete_type blind to alias sugar** (`clang/lib/AST/ExprConstantMeta.cpp`).
  `findTypeDecl` on a TypedefType returns the alias decl, for which EnsureInstantiated is a
  no-op, so an instantiable-but-never-instantiated spec named through an alias (spdlog's
  `stdout_sink_mt`, member typedefs) read as incomplete — order-dependently. Bit the
  binder's new completeness gates (BINDER-0014). Fix desugars like `members_of` does.
  Repro: `corpus/findings/repros/TC-0012/`; regression test
  `is-complete-type-alias-sugar.pass.cpp`. Upstreamed as issue #304 / PR #307 (tracking
  issue #308). (Rebuild for any of
  TC-0010..0012: `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.)
- **TC-0013 — dependent splice as `auto`-NTTP argument in a requires-expression ICEs at parse**
  (`clang/lib/AST/ExprClassification.cpp`). A dependent `CXXSpliceExpr` is created with a
  NULL model expression (`SemaReflect.cpp`, `BuildReflectionSpliceExpr`'s dependent tail);
  classifying it — `Sema::DeduceAutoType` deducing an `auto` NTTP from
  `typename probe<([:mem:])>` inside a requires-expression — dyn_cast'ed the null model:
  `Casting.h:662 dyn_cast on a non-existent value`, at PARSE time. Found by the DRIVER
  implementing BINDER-0020's value-readability probe (wave 1), not by a corpus run; the
  binder dodges it with a fixed-type (`long double`) NTTP probe. Fix: classify a model-less
  splice by its own value kind. Repro: `corpus/findings/repros/TC-0013/`; regression test
  `auto-nttp-dependent-splice-requires.pass.cpp`. (Rebuild:
  `ninja -C toolchain-build clang && ninja -C toolchain-build install-clang`.)

## Build & test the binder (the main loop) — verified from scratch

```bash
cd ~/git/cpp26-reflect-nanobind
TC=$PWD/toolchain                                              # the repo-local compiler
python3.12 -m venv .venv && .venv/bin/pip -q install pytest   # /opt/homebrew/bin/python3.12
cmake -S nanobind -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=$TC/bin/clang -DCMAKE_CXX_COMPILER=$TC/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DPython_EXECUTABLE="$PWD/.venv/bin/python" \
  -DNB_TEST=ON -DNB_TEST_FREE_THREADED=OFF -DNB_TEST_STABLE_ABI=OFF \
  -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath,$TC/lib" \
  -DCMAKE_MODULE_LINKER_FLAGS="-Wl,-rpath,$TC/lib"

ninja -C build test_reflect_ext test_reflect_codegen_ext
DYLD_LIBRARY_PATH=$TC/lib PYTHONPATH=$PWD/build/tests \
  .venv/bin/python -m pytest nanobind/tests/test_reflect.py \
  nanobind/tests/test_reflect_codegen.py -W error::RuntimeWarning
```
All 54 reflection tests pass. (`test_reflect_codegen_ext` exercises the two-stage codegen:
CMake builds a generator, runs it to emit a trampoline header, then builds the module that
includes it.)

## Build & run the standalone demos

```bash
cd ~/git/cpp26-reflect-nanobind
TC=$PWD/toolchain
$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
  -L $TC/lib -Wl,-rpath,$TC/lib examples/serialize_poc.cpp -o examples/serialize_poc
./examples/serialize_poc          # JSON + binary round-trip, all reflection-driven
```
`examples/refl.cpp` builds the same way and prints a struct's fields.

## Status & roadmap

Implemented in the binder: classes/ctors/data/static/methods (+ overloads), full
function-type qualifier matching (`const`/`noexcept`/`&`; skips `volatile`/`&&`/variadic),
operators→dunders (member + unary/binary free operators, incl. reversed dunders; widest
integral conversion wins `__int__`), enums, inheritance under the **reachability rule** (a
base is the real Python base only when independently in the bind set — looked up through
unbound links; everything else **flattens**, incl. whole internal facade chains), virtual
overrides (two-tier trampolines), annotation-driven control (skip/rename/doc/rv_policy/
keep_alive; doc covers classes/enums too), keyword-argument names (P3096 → nb::arg, incl.
constructors; default-argument *values* are a C++26 standard gap, not bindable), STL
type-caster coverage (codegen emits the needed <nanobind/stl/*.h> includes; header-only path
static_asserts the missing one), properties (annotated getter/setter pairs → def_prop_rw/ro),
templates (class-template *specializations* auto-discovered from public-member signatures to
a fixpoint — a spec's own template args do NOT qualify — + listed explicitly in the
`reflect_<...>` pack; CamelCase Python names like `Box<int>`→`BoxInt`), **member function
templates** with all-defaulted parameters (default instantiation under the template's name —
hash/btree heterogeneous query APIs incl. `operator[]`→`__getitem__`), and
**using-redeclarations** via entity proxies (incl. from PRIVATE bases, e.g. StatusOr's
`value()`; needs `-fentity-proxy-reflection`), **deleted-function filtering** on every
binding/scanning path (a class with only deleted ctors binds with no `__init__` → TypeError;
BINDER-0012), **Python-side copy construction** (`init<const T&>` for copyable non-trampolined
classes; BINDER-0013), **deduction-guide stripping** in the namespace walks (guides are
never bindable and pre-TC-0008 toolchains can't mangle their reflections), and **call-site
exclusions + completeness gates** (BINDER-0014, the eigen run: `nb::exclude_<...>` makes
templates/types/namespaces/individual members opaque on every path — what tames Eigen's
divergent expression-template discovery and its lazily-ill-formed shape-asserting member
bodies; non-completable specs are auto-skipped). Remaining: per-arg
ownership transfer, container `__iter__`/make_iterator, member templates needing explicit
args, trampoline hardening, friendly spec naming. The binder's git history on `mk-reflect`
has one commit per feature; `nanobind/docs/reflection.rst` is the user-facing reference.

## Gotchas (carried over; see submodule CLAUDE.md files for detail)

- **Spliced-lambda mangler crash** in clang-p2996: never put a spliced type
  (`[:type_of(x):]`) in a lambda *signature* passed to a dependent `cls.def*` call. The
  binder works around this throughout.
- **Annotation values must be valid template arguments** (no `const char*` members; strings
  use a `fixed_string<N>`).
- Everything needs the p2996 toolchain; both the generated trampolines and the binder use
  splices, so even generated code is compiled by the repo-local `./toolchain`.

## Working agreements

- **Upstreaming toolchain fixes (mandatory flow for EVERY new TC-XXXX):** (1) minimized
  repro under `corpus/findings/repros/TC-XXXX/` + finding write-up + regression test in
  `llvm-project/libcxx/test/std/experimental/reflection/` (or `clang/test/Reflection/`);
  (2) cherry-pick the fix commit onto bloomberg's `p2996` tip on a branch of
  `Cfretz244/llvm-project` (strip internal TC-/BINDER- references from the commit
  message; code comments must already be clean), validate the PR-state compiler (repro +
  regression test + `clang/test/Reflection` unchanged vs base), push, file issue + PR
  against `bloomberg/clang-p2996`; (3) **add the new issue/PR pair to the campaign
  tracking issue [bloomberg/clang-p2996#308](https://github.com/bloomberg/clang-p2996/issues/308)
  under its root-cause category** (A identity/equality, B sugar-blind metafunctions,
  C member/namespace enumeration, D reflection-NTTP mangling, E constant-evaluator
  robustness — add a category if none fits), and tick its checkbox there when the PR
  merges; (4) record everything in `repros/TC-XXXX/UPSTREAM.md` (FILED header with
  numbers + validation evidence — see TC-0005 or TC-0010 for the format).
- Binder changes: edit in `nanobind/` on `mk-reflect`, push to its `fork` remote
  (`Cfretz244/nanobind`); never push to `origin` (upstream wjakob).
- After updating a submodule, `git add <submodule> && git commit` here to re-pin the state.
- Submodule URLs: `nanobind` → `Cfretz244/nanobind`; `llvm-project` → `Cfretz244/llvm-project`
  (both GitHub forks). The umbrella itself lives at `Cfretz244/cpp26-reflect-nanobind`. A fresh
  `--init --recursive` clones cleanly on any machine. When re-pinning llvm-project, push its
  branch to the fork first so the new pin is reachable.
- `build/` and `.venv/` are git-ignored scratch — safe to delete and rebuild from scratch.
