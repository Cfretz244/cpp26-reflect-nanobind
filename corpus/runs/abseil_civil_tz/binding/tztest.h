// Fixture namespace for the Abseil civil-time/TimeZone run: thin inline wrappers
// over absl's free conversion functions (they live in ^^absl, not tractable to
// reflect whole; the timetest pattern). The civil types, TimeZone, Time, and
// Duration themselves bind head-on in binding.cpp.
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

namespace tztest {

// Zone factories. load() folds LoadTimeZone's bool+out-param into an optional.
inline absl::TimeZone utc() { return absl::UTCTimeZone(); }
inline absl::TimeZone fixed(int seconds_east) {
    return absl::FixedTimeZone(seconds_east);
}
inline std::optional<absl::TimeZone> load(std::string_view name) {
    absl::TimeZone tz;
    if (absl::LoadTimeZone(name, &tz)) return tz;
    return std::nullopt;
}

// Absolute <-> civil conversions.
inline absl::Time from_civil(absl::CivilSecond cs, absl::TimeZone tz) {
    return absl::FromCivil(cs, tz);
}
inline absl::CivilSecond to_civil_second(absl::Time t, absl::TimeZone tz) {
    return absl::ToCivilSecond(t, tz);
}
inline absl::CivilDay to_civil_day(absl::Time t, absl::TimeZone tz) {
    return absl::ToCivilDay(t, tz);
}

// Absolute-time anchors (no public integer ctors on Time).
inline absl::Time from_unix_seconds(std::int64_t n) {
    return absl::FromUnixSeconds(n);
}
inline std::int64_t to_unix_seconds(absl::Time t) { return absl::ToUnixSeconds(t); }

// Formatting (RFC3339, seconds precision) and weekday (enum -> index).
inline std::string format_sec(absl::Time t, absl::TimeZone tz) {
    return absl::FormatTime(absl::RFC3339_sec, t, tz);
}
inline int weekday_index(absl::CivilDay d) {
    return static_cast<int>(absl::GetWeekday(d));
}

}  // namespace tztest
