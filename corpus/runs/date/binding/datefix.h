// Fixture for the date run. Bound head-on are date's own value types
// (year/month/day, year_month_day, year_month, month_day, weekday). What the real
// API CANNOT express to Python:
//   * sys_days is std::chrono::time_point<system_clock, days> -- chrono time_points
//     have no nanobind caster and date::sys_days is a typedef, not a class the binder
//     can bind. The round-trip year_month_day <-> sys_days (the serial-day count that
//     underpins all calendrical arithmetic) is therefore exposed via free factory/
//     accessor functions over a plain int day-count.
//   * weekday(sys_days) -- the weekday-of-a-date computation -- likewise needs the
//     sys_days bridge.
//   * date::format()/operator<< are function TEMPLATES (CharT, Traits, Streamable);
//     a template can't be bound. format_*() wrappers stream the concrete value types
//     to an ostringstream with a concrete format string and hand back std::string.
// Everything here is a thin bridge over date's real API -- no reimplementation.
#pragma once
#include <date/date.h>
#include <chrono>
#include <sstream>
#include <string>

namespace datefix {

using date::year;
using date::month;
using date::day;
using date::weekday;
using date::year_month_day;
using date::days;

// --- sys_days bridge (serial day count since the 1970 epoch) ---

// year_month_day -> serial day count (sys_days since epoch).
inline long ymd_to_serial(const year_month_day& ymd) {
    date::sys_days sd = ymd;                 // operator sys_days()
    return sd.time_since_epoch().count();
}

// serial day count -> year_month_day.
inline year_month_day serial_to_ymd(long serial) {
    date::sys_days sd{days{serial}};
    return year_month_day{sd};               // year_month_day(sys_days)
}

// weekday of a calendar date, via the sys_days bridge (weekday(sys_days)).
inline weekday weekday_of(const year_month_day& ymd) {
    date::sys_days sd = ymd;
    return weekday{sd};
}

// --- formatted output bridges (date::format is a template) ---

inline std::string format_ymd(const std::string& fmt, const year_month_day& ymd) {
    return date::format(fmt, ymd);
}
inline std::string format_weekday(const std::string& fmt, const weekday& wd) {
    return date::format(fmt, wd);
}

} // namespace datefix
