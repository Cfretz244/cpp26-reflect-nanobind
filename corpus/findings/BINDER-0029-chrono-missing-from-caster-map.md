# BINDER-0029: std::chrono types missing from the STL caster map

- **Status**: fixed (nb_reflect.h `stl_caster_header`)
- **Found by**: the spdlog emit lane's Gate 6b surface diff -- the run's first
  D.surface. Both lanes were behaviorally E; the diff showed
  `logger::log(log_time: ...)` rendered as `datetime.datetime | ...` in the
  constexpr module but as the RAW
  `std::__1::chrono::time_point<system_clock, ...>` in the emit module.
- **Root cause**: `stl_caster_header` had no mapping for `std::chrono`
  templates, so the generated TU's auto-emitted caster includes lacked
  `<nanobind/stl/chrono.h>`. The constexpr lane masked the gap because
  spdlog's binding.cpp hand-listed the include (the header-only path's
  static_assert never fired for the same reason the emit TU compiled: an
  unconverted chrono param still BINDS, it is just uncallable/raw-typed).
- **Fix**: map `time_point` / `duration` -> `nanobind/stl/chrono.h`
  (std::chrono is nested under std; the is_in_std walk already accepts it).
  Shared layer: benefits the header-only diagnostic, the trampoline codegen's
  emitted includes, and the emit backend alike.
- **Lesson**: this is precisely the divergence class Gate 6b exists for --
  a behaviorally-green module with a silently degraded signature surface.
