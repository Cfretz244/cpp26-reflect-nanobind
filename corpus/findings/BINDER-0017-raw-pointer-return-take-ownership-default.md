# BINDER-0017 — raw class-pointer returns defaulted to take_ownership, double-freeing borrowed pointers

- **Status:** FIXED (binder, `nb_reflect.h`: `returns_raw_class_pointer` / `effective_rv_policy`)
- **Found via:** corpus/runs/cli11 (wave 1, THE D-blocker; dedup key
  `reflect-raw-ptr-return-take-ownership-default`). CLI11's whole fluent API returns borrowed
  pointers: `add_option`/`add_flag` → `Option*` owned by the App (`vector<unique_ptr<Option>>`),
  every setter returns `App*`/`Option*` self.
- **Symptom:** compiles + imports cleanly; crashes at runtime. A retained `add_option()` return →
  SIGABRT double free at GC/teardown; a *discarded* return → the Option freed immediately, the next
  `parse()` segfaults walking dangling pointers (`EXC_BAD_ACCESS at 0x8` in
  `CLI::App::_process_requirements`).
- **Root cause:** with no `[[=reflect::return_policy]]` annotation the binder passed
  `rv_policy::automatic`, which nanobind resolves to **take_ownership** for pointer returns. Hand
  bindings choose per-def; a reflection generator must pick a survivable default, and borrowed
  returns dominate real APIs.
- **Fix:** when un-annotated and the return is a bare pointer to a class type, the binder now binds
  `reference_internal` (instance methods — pointee lifetime tied to self) or `reference`
  (static/free functions). Explicit annotations still win; smart-pointer/by-value returns keep
  `automatic`. Trade-off: an ownership-TRANSFERRING raw return now leaks unless annotated
  `take_ownership` — a leak is strictly better than the double free, and transfer-by-raw-pointer is
  the rare case in modern C++.
- **Verification:** `test43_raw_pointer_return_borrows` (retained + discarded returns, fluent self,
  static/free reference returns); cli11 re-gated **D→E** (its 4 differential scenarios all pass).
