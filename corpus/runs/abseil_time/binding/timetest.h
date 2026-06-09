// Fixtures for the Abseil time run. absl::Duration / absl::Time have no public integer
// constructors — values are made by free factory functions (absl::Seconds/Minutes/Hours/
// FromUnixSeconds) and read back by free conversion functions (absl::ToInt64Seconds/...), which
// reflecting the *types* doesn't bind. These thin wrappers expose those real free functions so
// Python can construct/extract; the REAL Duration/Time arithmetic and comparison operators bind
// directly on the bound types (this file is scaffolding, not the thing under test).
#pragma once
#include <cstdint>
#include "absl/time/time.h"

namespace timetest {

// construction (factory free functions)
inline absl::Duration seconds(std::int64_t n)      { return absl::Seconds(n); }
inline absl::Duration minutes(std::int64_t n)      { return absl::Minutes(n); }
inline absl::Duration hours(std::int64_t n)        { return absl::Hours(n); }
inline absl::Duration milliseconds(std::int64_t n) { return absl::Milliseconds(n); }

// extraction (conversion free functions)
inline std::int64_t to_seconds(absl::Duration d)      { return absl::ToInt64Seconds(d); }
inline std::int64_t to_minutes(absl::Duration d)      { return absl::ToInt64Minutes(d); }
inline std::int64_t to_milliseconds(absl::Duration d) { return absl::ToInt64Milliseconds(d); }

// time points
inline absl::Time   from_unix_seconds(std::int64_t n) { return absl::FromUnixSeconds(n); }
inline std::int64_t to_unix_seconds(absl::Time t)     { return absl::ToUnixSeconds(t); }

}  // namespace timetest
