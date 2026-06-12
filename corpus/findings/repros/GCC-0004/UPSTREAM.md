# GCC-4 upstream record — probe-09 family: GCC IS CONFORMING on both layers

- status: **RECLASSIFIED — NOT A GCC BUG; do not file** (2026-06-12).
  The divergence is clang-p2996's type-trait predicates returning false
  for non-type reflections, against the adopted wording.
- repros: `gcc16-proveout/probes/09_discarded_splice.cpp` (fails by spec),
  `09b_workaround.cpp` (the conforming spelling, passes everywhere),
  and the isolating hybrid `gcc4_hybrid.cpp` (kept in this directory).

## Layer 1 — type-trait predicates are PARTIAL by specification

[meta.reflection.traits] (adopted C++26): "Every function and function
template declared in this subclause throws an exception of type
meta::exception unless ... for every parameter p of type info, is_type(p)
is true." So `is_class_type(^^api::fn)` (a function reflection) must NOT
return false — it must fail to be a constant, exactly what GCC does:

```
error: uncaught exception of type 'std::meta::exception';
'what()': 'reflection does not represent a type'
```

clang-p2996 returns false (earlier-revision behavior); probe 09's
ungated `classify` is written to that, i.e. the probe expectation itself
is nonconforming. The 09b gate (`is_type(m)` first) is the CORRECT
portable spelling, not a workaround.

## Layer 2 — discarded if-constexpr branches are NOT checked (no bug)

The original GCC-4 reading ("GCC checks splices in discarded if-constexpr
branches of expanded template-for bodies") was a misattribution caused by
cascade: when `constexpr kind k = classify(mem);` fails (layer 1), the
if-constexpr conditions are erroneous, no branch is discarded, and the
splice errors ("'api::fn' is not usable in a splice type") surface for
every branch.

Isolation (hybrid probe: conforming is_type-gated classify, but the
`typename [:mem:]` splices kept AT THE DISPATCH SITE in the discarded
branches): **passes on released 16.1 and on trunk** — class/enum/fn all
dispatch, exit 0. GCC discards correctly once the conditions are valid
constants.

## Consequences

- Nothing to file on gcc.gnu.org for either layer.
- The binder's dispatch idiom is safe on GCC as long as classification is
  is_type-gated (it is — the nb_annotations_of_type shim family already
  follows the gate-first discipline).
- Optionally reportable against clang-p2996 as a conformance divergence
  (predicates should throw, not return false). Low priority.
- Probe 09 stays failing BY DESIGN as a divergence canary; its header now
  records the conformance verdict. If a future GCC starts PASSING probe
  09 verbatim, that would be the regression to look at.
