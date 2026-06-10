# BINDER-0023 — `const void*` returns hard-errored in nanobind's void* caster

- **Status:** FIXED (binder: cv-qualified `void*` joined `is_unbindable_shape`).
- **Found via:** corpus/runs/sqlitecpp (wave 2; `Column::getBlob() -> const void*`; dedup
  key `binder-const-void-ptr-return-caster-hard-error`). nanobind's capsule caster takes
  plain `void*` only; the const mismatch was a TU-wide hard error instead of a skip.
- **Verification:** `test46_unbindable_cv_void_ptr` (Gadget::blob absent, class binds).
