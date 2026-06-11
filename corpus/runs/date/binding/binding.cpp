// Single-stage bindings for HowardHinnant/date v3.0.4 (Tier 2: value types + a rich
// operator surface). The calendrical value classes are bound HEAD-ON from the date
// namespace -- year/month/day, year_month/month_day, year_month_day, weekday -- so their
// real ctors, accessors, ok(), and the free comparison/arithmetic operators (operator==,
// operator< ..., operator+/- with days/months/years) map to Python dunders. The datefix
// fixture bridges only what the real API can't hand to Python: the sys_days serial-day
// round-trip (chrono time_point, no caster) and date::format() (a function template).
// Bind set defined once in binding_args.h (shared with the emit-lane generator).
#include <nanobind/nb_reflect.h>
#include "binding_args.h"
#include <nanobind/stl/string.h>
// BINDER-0029 made the caster walk see date's chrono-typed signatures
// (sys_days/local_days time_points), so the header-only diagnostic now
// demands the chrono caster this TU always needed:
#include <nanobind/stl/chrono.h>

namespace nb = nanobind;

NB_MODULE(date_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
