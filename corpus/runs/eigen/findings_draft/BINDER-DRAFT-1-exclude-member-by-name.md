# BINDER-DRAFT-1 — nb::exclude_member_<Owner, "name">: GCC-safe by-name member exclusion (GCC-6 at the member-exclusion site)

- **dedup_key**: gcc6-exclude-member-by-name
- **run**: eigen (Eigen 5.0.1; the BINDER-0014 flagship)
- **backend**: gcc16
- **related GCC finding**: GCC-0006 (dedup_key gcc6-constexpr-member-lift),
  probe `gcc16-proveout/probes/xfail_gcc6_constexpr_member_lift.cpp` — this is a
  NEW SITE of the same divergence, recorded against GCC-0006 (no new GCC finding).

## Symptom

eigen was green on clang (E/E, surface pass) but outcome **B (B.compile)** on
GCC 16 at gate 4 — both lanes. The constexpr binding failed before any binding
ran: forming the run's `nb::exclude_<...>` marker itself was the hard error,
because the marker lists the exact reflections of Eigen's lazily-ill-formed
member functions (the BINDER-0014 per-member escape hatch). Library-header
errors, not binder errors, e.g.:

```
Eigen/src/Core/DenseCoeffsBase.h:396: error: static assertion failed: OUT_OF_RANGE_ACCESS
  constexpr ... w() ... [with Derived = Eigen::Matrix<double, 3, 1>]
... required from reflect_constant(w-reflection)   (the exclude_ marker's add() lambda)
Eigen/src/Core/DenseCoeffsBase.h:355: THE_BRACKET_OPERATOR_IS_ONLY_FOR_VECTORS  (operator[] on a 3x3)
PlainObjectBase.h:302: YOU_TRIED_CALLING_A_VECTOR_METHOD_ON_A_MATRIX           (resize on a 3x3)
```

## Root cause

Two coupled GCC-side divergences (NOT binder logic bugs):

1. **GCC-6 (catalog item 6/11; GCC-0006):** GCC 16 instantiates a `constexpr`
   member's BODY the moment its reflection is materialized as an NTTP
   (`reflect_constant`, or the `define_static_array` lift). Eigen's vector-only
   accessors `w()`/`x()`/`y()`/`z()`/`operator[]` and the vector-`resize()` are
   `constexpr` and `static_assert` a shape in their bodies, so they are lazily
   ill-formed on the wrong-shaped bound spec.
2. The BINDER-0014 member-level `nb::exclude_<...>` escape hatch works by listing
   each problematic member's **reflection** (obtained via `members_of`) — which
   on GCC instantiates exactly the bodies it is trying to exclude. The exclusion
   mechanism collides head-on with GCC-6: the member cannot be named by
   reflection at all, and `excluded_v` (a `define_static_array`) cannot hold it.
   Confirmed minimal: a member info lives freely in a `constexpr` value, but
   `reflect_constant(info)` / `holder<info>` / `define_static_array({info})` each
   instantiate the body. The `expected` run's BINDER-DRAFT-1 anticipated this
   ("does NOT, and cannot, drop a mapped/ordinary member whose constexpr body is
   lazily ill-formed; no such member arises in practice because libraries
   enable_if them away") — Eigen is exactly that case (in-body `static_assert`,
   not `enable_if`), so the reflection-NTTP exclusion route is GCC-incompatible.

The binder's OWN base-flatten lift (`flatten_base_members` →
`define_static_array(liftable_class_members(Base))`) ALSO instantiates these
bodies on GCC, because the facade bases (`DenseCoeffsBase<Matrix,N>` etc.) are
bound as real Python bases and their member list is lifted before any per-member
exclusion gate runs. So even with the marker fixed, the binder lift bites.

## Fix summary

A new general binder capability: exclude a member **by name**, without ever
forming its reflection.

- **New marker** `nb::exclude_member_<std::meta::info Owner, fixed_string Name>`
  (nb_reflect.h). `Owner` is a CLASS reflection (safe to materialize — no member
  body is instantiated) and `Name` a `fixed_string` NTTP. Passed inside
  `exclude_<...>` exactly like an entity reflection. Honored on BOTH backends, so
  one marker set serves clang and GCC and the surfaces stay identical.
- **Rule extraction** (`compute_excluded_members` / `excluded_members_v`,
  nb_reflect.h): collects `(owner, name)` from each `exclude_member_` marker at
  collection time (where the marker is a constant, so its `fixed_string` NTTP can
  be spliced; the name is copied into a `define_static_string` so the
  `{info, const char*}` rule is structural and survives `define_static_array`).
  Reads only the markers' own template arguments — never the named members'
  reflections.
- **Pre-lift drop** (`liftable_class_members(cls, derived, rules)`,
  nb_reflect.h): the single drop point. A member is dropped before the lift when
  a rule's owner equals `derived` OR is a class in the owner's public-base
  subtree (the facade bases bound as real Python bases must drop the member too,
  not just the flattened path). Operator members (no identifier) are matched by
  their Python dunder — `operator[]` is named `"__getitem__"`. Threaded into all
  lift sites in `bind_class_contents` / `flatten_base_members` (nb_reflect.h) and
  the emit backend's `append_class_contents` / `append_flatten_base` /
  `probe_member_fns` (nb_reflect_emit.h). WHAT-to-bind logic lives only in the
  shared classifier; the emitter just calls it.

Because the drop happens on both backends, the bound surface is unchanged on
clang (the members were already excluded there by reflection); the change merely
moves the decision to a name and keeps the lazily-ill-formed bodies out of the
lift on GCC.

## Files touched

- `nanobind/include/nanobind/nb_reflect.h` — `exclude_member_` marker;
  `member_excl_rule` + `is_exclude_member_marker` + `resolve_exclude_member` +
  `compute_excluded_members` + `excluded_members_v` + `member_excluded_by_name`;
  `liftable_class_members` 3-arg overload (pre-lift drop) + the 1-arg shim;
  threaded rules into the 4 lift sites in `bind_class_contents` /
  `flatten_base_members`.
- `nanobind/include/nanobind/nb_reflect_emit.h` — threaded the rules + derived
  class into the 5 emit lift sites (`append_class_contents` ×3,
  `append_flatten_base`, `probe_member_fns`).
- `nanobind/tests/test_reflect_fixture.h` + `test_reflect_args.h` +
  `test_reflect.cpp` + `test_reflect.py` — fixture member `XVec::named_out`
  excluded by `nb::exclude_member_<^^XVec, "named_out">`; static_asserts on the
  rule table + `member_excluded_by_name`; Python surface assertion.
- `corpus/runs/eigen/binding/binding_args.h` (run-local) — the 5 instantiating
  member families (`w` on all 4 specs; `x`/`y`/`z`/`__getitem__` on Mat3; `y`/`z`
  on CVec1; `resize` on all 4) moved from reflection-listing to
  `nb::exclude_member_`; the now-unused `collect_subscript_operators` removed.

## Validation

- eigen run gcc16: `outcome=E constexpr=E emit=E surface=pass` (matches clang
  result.json: E/E, surface pass; 32 top-level objects compared, 0 mismatch).
  constexpr 72.6 s, emit 93.1 s.
- eigen run clang (re-verified, result.json restored afterward, not edited):
  still `outcome=E constexpr=E emit=E surface=pass` — the shared-source change
  keeps clang green and the surface identical.
- Unit suites after the edit: GCC container 129 passed, clang host 129 passed
  (with the new XVec::named_out fixture case in both).
