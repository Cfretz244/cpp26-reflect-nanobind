# BINDER-0011 — abstract classes bound nb::init; skipped methods demanded casters

- **Status:** FIXED (binder commit alongside the spdlog run)
- **Found via:** `corpus/runs/spdlog` (Tier 3). Binding `spdlog::sinks::sink` head-on
  failed at Gate 4 twice over:
  1. `allocating an object of abstract class type 'sink'` out of `cls.def(init<>())` —
     the binder bound constructors for an **abstract** class.
  2. After fixing (1): the caster-matrix `static_assert` demanded
     `<nanobind/stl/unique_ptr.h>` for `sink::set_formatter(std::unique_ptr<formatter>)`
     — a method the binder **never binds** (by-value move-only param, BINDER-0010).
- **Files:** `nanobind/include/nanobind/nb_reflect.h`.

## Root causes

1. **Abstract-class constructors.** `bind_class_contents` bound every public
   non-template constructor via `nb::init<...>`, whose dispatch lambda instantiates `T`
   — ill-formed when `T` has pure virtuals. Any abstract interface anywhere in the bind
   set was a TU-wide hard error (the first one a corpus library hit: spdlog's sink
   hierarchy; Abseil's surfaces had no abstract bases).

2. **Caster matrix over-collection.** `collect_own_stl_member_types` (the
   `required_stl_types` walk behind the header-only path's `check_stl_casters`
   static_assert and codegen's `emit_stl_includes`) collected return/parameter types
   from **every** public function member, without the bind-path skip predicates. A
   method skipped at bind time ([[=reflect::skip]] or BINDER-0010 by-value move-only)
   still demanded casters for a signature that never binds.

## Fixes

1. The constructor pass runs under
   `if constexpr (!std::is_abstract_v<T> || has_reflect_trampoline<T>)`. An abstract
   class binds with **no** ctors — Python instantiation raises TypeError; concrete
   descendants still get it as their real Python base. The trampoline exception is
   load-bearing: with a registered Alias, `nb::init` constructs the **trampoline**
   (that is how a Python subclass overriding pure virtuals is instantiated) — the
   codegen tests' abstract `Worker`/`Processor<int>` cover it.

2. `collect_own_stl_member_types` / `collect_scope_stl_types` now mirror the bind-path
   predicates (`fn_skip_annotated`, the value-form of `has_ann<fn, reflect::skip>`, and
   `has_move_only_by_value_param`) on all four branches: plain methods, entity proxies,
   member-template default instantiations, and namespace free functions.

Tests: `test27d_abstract_class_no_ctor` (abstract base binds, no ctor, derived
`isinstance`; its skipped `std::unique_ptr<int>` signatures compile without the
unique_ptr caster included). The spdlog run is the integration case: `sink` (4 pure
virtuals incl. `set_formatter(unique_ptr<formatter>)`) binds as the Python base of
`stdout_sink_mt`, no unique_ptr caster needed.

dedup_key: `abstract-class-ctor`.
