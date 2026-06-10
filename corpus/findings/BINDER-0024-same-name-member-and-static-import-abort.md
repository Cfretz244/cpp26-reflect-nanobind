# BINDER-0024 — a static method shadowed by a same-named instance method aborted nanobind at import

- **Status:** FIXED (binder: `instance_method_shadows` gate on the static-method paths,
  incl. member-template default instantiations).
- **Found via:** corpus/runs/sqlitecpp (wave 2; `SQLite::Database::getHeaderInfo` — const
  member + static overload; dedup key `binder-same-name-member-static-method-import-abort`).
  Worst-case shape: clean compile, SIGABRT at import during type finalization (nanobind
  cannot make one attribute both an instancemethod and a staticmethod).
- **Fix:** the instance method wins; the shadowed static is skipped.
- **Verification:** `test48_static_shadowed_by_instance_method` (module imports; instance
  path works; unshadowed statics still bind).
