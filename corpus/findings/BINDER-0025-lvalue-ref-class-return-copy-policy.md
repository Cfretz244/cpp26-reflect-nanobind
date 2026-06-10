# BINDER-0025 — T& class returns defaulted to rv_policy::copy (abort for non-copyable, silent detach otherwise)

- **Status:** FIXED (binder: `returns_raw_class_pointer` generalized to
  `returns_borrowed_class_indirection` — lvalue references join BINDER-0017's borrowing
  default).
- **Found via:** corpus/runs/taskflow (wave 2; dedup key
  `lvalue-ref-class-return-defaults-to-copy-policy`): accessors returning
  `tf::Taskflow&`/`tf::Executor&` (move-only) aborted at runtime
  (`nb_type_put_common(..., policy=copy)`); for copyable T the same shape silently
  returned a DETACHED copy — wrong sharing semantics for an accessor.
- **Fix:** un-annotated `T&` class returns bind `reference_internal` (methods) /
  `reference` (statics & free functions); annotations win.
- **Verification:** `test50_lvalue_ref_return_borrows` (non-copyable holder; mutation
  through the returned view is visible). The taskflow run's pointer-returning fixture
  accessors are now removable as a residual.
