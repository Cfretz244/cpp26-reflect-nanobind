// Single-stage bindings for HowardHinnant/date v3.0.4 (Tier 2: value types + a rich
// operator surface). The calendrical value classes are bound HEAD-ON from the date
// namespace -- year/month/day, year_month/month_day, year_month_day, weekday -- so their
// real ctors, accessors, ok(), and the free comparison/arithmetic operators (operator==,
// operator< ..., operator+/- with days/months/years) map to Python dunders. The datefix
// fixture bridges only what the real API can't hand to Python: the sys_days serial-day
// round-trip (chrono time_point, no caster) and date::format() (a function template).
#include "datefix.h"

#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

NB_MODULE(date_ext, m) {
    nb::reflect_<
        ^^date::year, ^^date::month, ^^date::day,
        ^^date::weekday,
        ^^date::year_month, ^^date::month_day,
        ^^date::year_month_day,
        ^^datefix>(m);
}
