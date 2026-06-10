# BINDER-0021 — a data member of an uncastable type binds as a property that always throws (OPEN, design note)

- **Status:** OPEN — deliberately deferred; current behavior matches nanobind's documented
  "no caster ⇒ runtime TypeError" model.
- **Found via:** corpus/runs/fast_float (wave 1, low confidence; dedup key
  `binder-binds-data-member-of-uncastable-enum-type-as-unreadable-property`).
  `from_chars_result_t<char>::ec` is `std::errc` — a std scoped enum with no caster and not in
  the bind set — so the generated accessor raises `TypeError: Unable to convert function return
  value to a Python type!` on every access.
- **Why deferred:** "this member's type can never convert" is not decidable in general at bind
  time (casters are an open set; a type may be registered later in module init). The two candidate
  improvements, for a future pass:
  1. Pull a member's scoped-enum type into the enum-binding pass when it is enumerable
     (std::errc would then bind as a real enum) — the more useful fix.
  2. Narrow skip: omit data members whose type is a std scoped enum with no caster header mapped
     (reuses the `stl_caster_header`/`is_in_std` machinery).
- **Run impact:** none — fast_float reached E; the run locks the raising behavior in as a test so
  any future change here will surface.
