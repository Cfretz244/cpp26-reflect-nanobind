# BINDER-0019 — forward-declared plain classes in signatures (and namespaces) escaped the completeness gates

- **Status:** FIXED (binder, `nb_reflect.h`: plain-class branch in `type_mentions_excluded` +
  `is_complete_type` guards in the namespace walks)
- **Found via:** corpus/runs/pugixml (wave 1; dedup key
  `binder-no-completeness-gate-for-incomplete-pointer-in-member-signature`). pugixml's handle
  classes expose their pImpl pointers: `xml_node_struct* internal_object() const` and
  `explicit xml_node(xml_node_struct*)` over structs that are forward-declared by design.
- **Symptom:** binding `^^pugi::xml_node` head-on hard-errored deep inside nanobind/libc++
  (`'typeid' of incomplete type`, `incomplete type used in type trait expression`) with no
  binder-level diagnostic. The pugixml agent worked around it with `nb::exclude_` on the two
  structs (enumerated by hand).
- **Root cause:** BINDER-0014's completeness gate only covered user class-template
  *specializations*; a non-template `struct foo;` reached nanobind's caster machinery. The
  namespace walks had the same blind spot: a forward-declared namespace member
  (`struct Opaque;`) hit `members_of` on an incomplete type in the discovery/caster walks.
- **Fix:** `type_mentions_excluded` now also rejects a plain class type that is
  `!is_complete_type` (after the same dealias + pointer-stripping), making members that mention
  one skip on every bind path exactly like non-completable specs; the four scope walks
  (`reflect_dispatch`, `collect_seed_classes`, `collect_scope_user_specs`,
  `collect_scope_stl_types`) skip incomplete class members. Relies on TC-0012's alias-sugar fix
  for `is_complete_type`.
- **Verification:** `test42_unbindable_shapes_skip` (`impl()` over a never-defined `Opaque` skips;
  `Opaque` itself unbound); pugixml stays E and its hand-written `nb::exclude_` of the pImpl
  structs is now redundant (left in place; harmless).
