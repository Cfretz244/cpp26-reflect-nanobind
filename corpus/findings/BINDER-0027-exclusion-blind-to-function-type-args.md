# BINDER-0027 — exclusions/completeness do not recurse into std::function's signature types (OPEN)

- **Status:** OPEN — deferred, low severity (dead uncallable surface, no crash).
- **Found via:** corpus/runs/httplib (wave 2; dedup key
  `exclude-and-completeness-not-recursing-into-stdfunction-arg-types`): a method taking
  `std::function<bool(DataSink&)>` still binds when `DataSink` is excluded, exposing the
  excluded type inside the `Callable[...]` doc signature; calling it raises a generic
  TypeError.
- **Fix direction (future):** `type_mentions_excluded` should decompose FUNCTION-TYPE
  template arguments (return + parameter types) when recursing into template-argument
  trees.
