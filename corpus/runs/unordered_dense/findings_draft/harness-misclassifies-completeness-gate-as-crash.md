dedup_key: harness-classifies-binder-static_assert-as-compiler-crash

Layer: HARNESS (corpus/lib/run_gates.py bug-classifier) -- NOT a binder/toolchain/library defect.

Observation (not a finding to upstream; recorded for the driver's triage):

During the unordered_dense bring-up, before the STL caster includes were added,
the binding TU failed Gate 4 with the binder's own BINDER-0014 completeness-gate
static_assert:

  nb_reflect.h:2238: static assertion failed due to requirement
  '!is_base_caster_v<...type_caster<std::pair<iterator,bool>,int>>':
  nb::reflect_: a bound signature uses the std type 'pair<...>', whose nanobind
  type caster is not included in this translation unit. Add:
  #include <nanobind/stl/pair.h>

run_gates.py classified this as:
  toolchain_bug = {status: suspected, kind: compiler-crash, signature: "assertion failed"}

This is a FALSE POSITIVE in the harness heuristic: a `static_assert` failure
text contains the substring "assertion failed", which the classifier treats as a
compiler self-assert/ICE. It is the intended, diagnostic-quality binder gate
firing (it even names the fix). The correct taxonomy was plain B.compile
(missing caster include), which the run reached and then cleared by adding
#include <nanobind/stl/{pair,vector,optional}.h>.

Smallest trigger: any reflect_ over a container whose method signatures mention
an STL type without that caster header included -> B.compile via the
completeness gate -> harness tags it `compiler-crash / "assertion failed"`.

Suggested (for the driver, since corpus/lib is off-limits to run agents): the
classifier should exclude lines matching `static assertion failed` /
`static_assert` originating from nb_reflect.h before matching the generic
"assert" -> compiler-crash rule.

Final state of this run: caster includes added; outcome E, toolchain_bug none.
