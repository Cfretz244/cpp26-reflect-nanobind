# BINDER-0004 — Redundant re-walk in the compile-time collection walks (heavy-type perf)

- **Status:** changed (perf de-dup) — **NOT a correctness fix** (see retraction below)
- **Found via:** Phase 1, nlohmann/json (`basic_json`)
- **File:** `nanobind/include/nanobind/nb_reflect.h` — the STL-caster and user-spec collection walks

## Retraction / correction

An earlier version of this finding (and the first nanobind commit message) claimed an
**"infinite recursion on self-referential types"** bug. **That was a misdiagnosis** — verified
empirically, not just reasoned:

- The walks (`collect_stl_types`, `collect_user_specs_from_type`) recurse over a type's
  **template arguments**, which form a **finite tree** — they cannot cycle. A class that refers to
  itself does so through **members** (e.g. `Node { std::vector<Node> children; }`), and these walks
  do **not** follow members recursively.
- Direct test: the self-referential fixture `corpus/runs/_fixture_recursive` (`Node` holding
  `std::vector<Node>`) **builds fine (~1 s) on the pre-change binder** (be1a43c) as well as after —
  i.e. self-reference never caused infinite recursion.
- The original json error was `constexpr evaluation hit maximum step limit`; "possible infinite
  loop?" is clang's **generic** message for *any* step-limit hit, not a confirmed loop.

So the cycle-guard framing was wrong. What the change actually is:

## What the change really does (and the real, demonstrated effect)

A **redundant-walk de-duplication / performance** change: a `visited` memo set, **shared across a
class's whole member walk**, so each type's argument graph is walked **once** instead of once per
member. Without it, a type like `basic_json` (~hundreds of members, each mentioning `basic_json`)
incurs O(members × graph) constexpr work.

**Demonstrated effect:** before the change, json failed in the **STL-caster walk**
(`collect_class_stl_types`) at the step limit; after, that walk **passes** and the failure moves to
the next stage (the user-spec fixpoint). So the de-dup is a real, measurable improvement for heavy
types — it just does not, by itself, make json buildable.

## Net assessment (honest)

- json is **still B**: `basic_json` remains too large for the toolchain's constexpr budget, and
  forcing past it ICEs the compiler (**TC-0002**). This change does not fix that.
- This change fixes **no reproducible correctness bug**; it is a perf de-dup that is safe
  (result-set identical) and helps large types. The `visited` guard is harmless but is **not**
  guarding a reachable cycle.
- Validation status: the perf effect is shown on json's STL walk (fail → pass). There is **no**
  before/after correctness regression it fixes. `corpus/runs/_fixture_recursive` is **coverage**
  for recursive types (reaches E), **not** a regression test for this change.
- Suite still passes 41/41; the four prior corpus runs still E.

## Upstream

Lands on `mk-reflect` as a perf change with corrected comments. The first commit message
(`a00551e`) overstated it as an infinite-recursion fix; a follow-up commit corrects the in-code
comments and this finding.
