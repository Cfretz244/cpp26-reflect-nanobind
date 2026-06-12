# RUN-DRAFT-1 — template_arguments_of throws on a `using`-alias reflection (GCC P3560 strictness)

- **dedup_key**: gcc-p3560-template-args-of-alias
- **run**: eigen
- **backend**: gcc16
- **catalog**: item 1 (P3560 strictness — GCC metafunctions throw
  `std::meta::exception` where the clang-p2996 fork was lenient)

## Symptom

The run's exclude-marker helper `collect_bad_matrix_ctors` did
`std::meta::template_arguments_of(M)[0]` where `M` was one of the run's `using`
aliases (`Vec3 = Eigen::Matrix<double,3,1>`, etc.). On GCC the marker
construction threw at compile time:

```
error: uncaught exception of type 'std::meta::exception';
       what(): 'reflection does not have template arguments'
```

surfacing as `reflect_<...>` deduction failure (the marker is a default argument
to the call).

## Root cause

`^^Vec3` is the ALIAS reflection. On GCC 16, `has_template_arguments(^^Vec3)`
is `false` and `template_arguments_of(^^Vec3)` THROWS — the spec's template
arguments are only visible after `dealias`. (`remove_cvref` dealiases on GCC, so
the binder's own `spec_camel_name` etc. are unaffected; the raw
`Eigen::Matrix<double,3,1>` spec passed in the reflect_ pack is direct and fine.
Only naming the spec through a `using` alias hits it.) The clang-p2996 fork
auto-dealiased, so the helper worked there.

## Fix (run-local)

`sm::template_arguments_of(sm::dealias(M))[0]` in `collect_bad_matrix_ctors`
(`corpus/runs/eigen/binding/binding_args.h`). `members_of`/`parameters_of` on the
alias already tolerate it — `template_arguments_of` is the one strict
metafunction here.

## Status

Run-local fix only. The GCC strictness itself is catalog item 1 (a documented
P3560 divergence, not a new finding); recorded here for the wave's dedup so the
supervisor sees the alias-specific shape. No binder change (the binder never
feeds an alias reflection to `template_arguments_of`).
