# BINDER-0014 — expression-template libraries: divergent discovery, non-completable specs, lazily-ill-formed bodies → nb::exclude_ + completeness gates

- **Status:** FIXED (binder feature + gates) — `nb::exclude_<...>` call-site
  exclusions and completeness-aware discovery/bind gates in
  `nanobind/include/nanobind/nb_reflect.h`; covered by the `exclude_test`
  fixture in `tests/test_reflect.{cpp,py}` (test41) and end-to-end by
  `corpus/runs/eigen`; documented in `docs/reflection.rst` ("Excluding
  entities")
- **Track:** binder
- **Found via:** `corpus/runs/eigen` (Eigen 5.0.1, Tier 5) — the first
  expression-template library in the corpus. Three compounding failure modes,
  none reachable by the existing `[[=reflect::skip]]` annotation (you do not
  own Eigen's headers):

## The three failure modes

1. **Divergent discovery fixpoint.** `required_user_specs` walks every
   discovered spec's member signatures (and its base subtree's). Eigen's CRTP
   facades mint NEW specializations from every walked one:
   `DenseBase<D>::transpose()` returns `Transpose<D>`, whose facade has
   `Transpose<Transpose<D>>`; `asPermutation()` grows `Matrix<N,N>` through
   `PermutationWrapper` to `Matrix<N*N,N*N>` (observed: `Matrix<double,
   3^16, 3^16>` before the step budget died). The fixpoint never closes.
2. **Specs that are hard errors to WALK.** `members_of` on
   `NestByValue<CwiseUnaryOp<...>>` instantiates member declarations whose
   `enable_if<HasDirectAccess,...>::type` is ill-formed outside SFINAE — a
   hard error no reflection query can pre-detect. And specs of templates
   forward-declared in the TU but defined in never-included headers
   (`SparseView` under `<Eigen/Dense>`, the `MatrixFunctionReturnValue`
   family from `unsupported/`) cannot be completed at all: walking them (or
   instantiating nanobind's caster on them at bind time) is a hard error.
3. **Lazily-ill-formed member BODIES.** Eigen declares shape-specific members
   on every Matrix and static_asserts the shape in the BODY:
   `Matrix(x, y, z)` on a 3x3, `operator[]`/`x()`/`y()`/`z()` on matrices,
   `w()` on 3-vectors, `eulerAngles` on vectors, `setZero(Index)` and the
   resize family on fixed-size specs, `begin()/end()` (whose iterator typedef
   is literally `void` on matrices)... `nb::init`/the method binders
   instantiate bodies, so each one is a TU-wide hard error. Bodies are not
   reflectable: NO query can detect this class.

## The fix (three layers)

- **`nb::exclude_<^^Entity...>`** — a marker type passed in the `reflect_`
  pack. Entries may be class templates (every specialization), concrete
  types, namespaces (everything inside, transitively), or **individual member
  reflections** (obtained via `members_of` — the only handle on failure mode
  3). Excluded entities are opaque everywhere: never seeds, never discovered,
  never walked, never flattened as bases, never demand casters, and any
  member whose signature mentions one (or that is listed itself) is
  gracefully skipped on every bind path. A spec PARAMETERIZED by an excluded
  type anywhere in its argument tree is transitively tainted.
- **Completeness gates** — a spec that cannot be COMPLETED is neither
  discoverable (`is_complete_type` gate in `is_user_class_template_spec`) nor
  bindable (the same probe in `type_mentions_excluded`, which all member
  gates consult). This is annotation-independent and protects every run.
  All checks DEALIAS first (member typedefs otherwise hide the entity —
  and on unpatched toolchains `is_complete_type` is itself sugar-blind,
  TC-0012).
- **Divergence guard** — `required_user_specs` caps the worklist at 1024
  specs and fails through the deliberately non-constexpr
  `reflect_discovery_diverged(last_spec)`, so a forgotten exclusion produces
  a pointed diagnostic instead of constexpr-step exhaustion.

## Corpus-wide effects (the regression sweep that validated it)

- The completeness probes raised consteval cost: four runs needed an
  explicit `-fconstexpr-steps` bump in `meta.toml` (abseil_statusor,
  abseil_containers, abseil_civil_tz, expected).
- The sweep exposed THREE latent toolchain bugs (TC-0010 re-opened-namespace
  reflection equality, TC-0011 namespace walk vs out-of-line member
  definitions, TC-0012 is_complete_type alias sugar) and one latent run
  regression: spdlog's intermediate sink base had silently stopped binding
  when BINDER-0012 landed (its only signature self-mentions are DELETED
  operators, which pull nothing into the bind set) — fixed by listing it
  explicitly in the pack, per the reachability rule's documented opt-in.
- Final state: all 20 corpus runs at outcome E (including `expected`,
  previously pending, whose 16 latent differential tests now pass).
