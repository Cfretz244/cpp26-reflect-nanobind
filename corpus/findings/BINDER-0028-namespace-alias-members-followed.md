# BINDER-0028 — namespace-alias members of a reflected namespace pulled the whole target namespace into the walk

- **Status:** FIXED (binder: all four scope walks skip `is_namespace_alias` members).
- **Found via:** corpus/runs/simdjson (wave 2, run-side observation): a fixture's
  convenience alias (`namespace sd = simdjson;`) inside the reflected namespace made the
  walk bind the entire library — discovery exploded into `ondemand::*` internals and
  exposed TC-0015. An alias is a shorthand, not a declaration of contents.
- **Verification:** `test49_namespace_alias_not_followed` (aliased namespace's classes stay
  unbound; the fixture namespace's own members bind).
