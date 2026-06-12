# GCC-0006 (+ GCC-1) upstream record — reflection lift instantiates member definitions

Covers BOTH probe-backed findings with the same root cause:

- **GCC-1**: lifting reflections of IMPLICITLY-declared special members into
  `define_static_array` synthesizes their definitions
  (`gcc16-proveout/probes/xfail_gcc1_implicit_member_lift.cpp`).
- **GCC-6 / finding GCC-0006**: the same lift instantiates a `constexpr`
  member's BODY; lazily-ill-formed bodies hard-error
  (`gcc16-proveout/probes/xfail_gcc6_constexpr_member_lift.cpp`).

- status: **PATCH READY** (not yet filed — filing is the user's call)
- patch: `0001-cxx-reflection-dont-instantiate-definitions-of-reflected-functions.patch`
  (commit `a2b10c8601f` on the devenv `proveout-fixes` branch, base master
  `7ce3a7b1beb`)
- affected: GCC 16.1 (release) and trunk 17.0 @ 7ce3a7b1beb (verified both;
  on trunk GCC-1 surfaces as the stl_construct.h overload error)

## Root cause (debugger-verified)

`cxx_eval_outermost_constant_expr` runs a P0859 pre-pass,
`instantiate_constexpr_fns` / `instantiate_cx_fn_r` (gcc/cp/constexpr.cc):
every constexpr FUNCTION_DECL *mentioned* by the expression is
`instantiate_decl`'d and every maybe-deleted defaulted function
`synthesize_method`'d before evaluation. The walker descends into
`REFLECT_EXPR` operands (REFLECT_EXPR is `EXPR_P`, and its operand is the
bare FUNCTION_DECL handle), so an evaluation that merely **materializes**
reflections — `define_static_array (members_of (^^T, ...))`, the canonical
lift — instantiates the definition of every reflected constexpr member and
synthesizes every implicit special member of T.

Backtrace (gdb on the GCC-1 probe): `synthesize_method` ←
`instantiate_cx_fn_r` (constexpr.cc:10744) ← `instantiate_constexpr_fns` ←
`cxx_eval_outermost_constant_expr` (:11028) ← `maybe_constant_value` ←
reflect.cc:683 (metafunction evaluation).

Conformance: P0859 applies to functions *named by* a potentially
constant-evaluated expression in the [basic.def.odr] sense. Forming a
reflection of a function is not such a naming (P2996); only evaluating
THROUGH the reflection (splice, `extract`) is — and those paths call
`mark_used` themselves, plus `cxx_eval_call_expression` has its own
"can't defer instantiating any longer" fallback (constexpr.cc:4314), so
skipping the pre-instantiation cannot under-instantiate.

Why only constexpr/implicit members bite: non-constexpr, non-defaulted
members fail the walker's own filter, which is why the GCC-6 probe's
`plainfn` (byte-identical ill-formed body, no `constexpr`) never errored.

## The fix

One hunk in `instantiate_cx_fn_r`: do not walk into `REFLECT_EXPR` subtrees.

## Field shapes (for the report's motivation)

- `tl::expected<void, E>`: unconditionally declared
  `constexpr operator->() const` returns `addressof(this->m_val)`; the void
  storage base has no `m_val`. Reflecting the specialization to generate
  bindings is a hard error. clang-p2996 binds it cleanly.
- Eigen: vector-only `constexpr` accessors (`x()`/`y()`/`z()`/`w()`,
  `resize()`) `static_assert` a shape in their bodies; reflecting a
  wrong-shaped bound specialization errors the same way.
- Any class holding `vector<unique_ptr<T>>`: the implicit copy operations
  are deleted-by-instantiation; the lift synthesizes them and errors.

## Verification (devenv, trunk @ 7ce3a7b1beb + patch)

- Both probes compile AND run (exit 0); `members_of` lift count correct.
- Reflection + constexpr testsuite slices: 3939 passes / 0 unexpected
  failures (run together with the GCC-0008 patch; see GCC-0008/UPSTREAM.md).
- New regression test `g++.dg/reflect/members-of-lift1.C` (both shapes).
- Probes 0*.cpp conformance smoke unchanged.

## Bugzilla material (component c++)

- Summary: `[C++26] -freflection: materializing a reflection as a constant
  instantiates the reflected function's definition (members_of +
  define_static_array hard-errors on lazily-ill-formed members)`
- Description: root cause section above; expected = clean (reflections are
  identities, bodies untouched; clang-p2996 accepts both probes); actual =
  hard error; note the is_deleted metafunction (eval_is_deleted) is the
  intentional place where deletedness gets resolved, the blanket pre-pass is
  not.
- Attach both probes. CC the reflection implementers (as in GCC-0008).
- Related cleanup the maintainers may want: `eval_is_deleted` already
  synthesizes on demand, so behavior of `is_deleted` is unchanged by this
  patch (verified by the testsuite slice).
