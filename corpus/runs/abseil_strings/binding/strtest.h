// Fixture namespace for the Abseil strings run. Abseil's classic string API is
// variadic/range templates (StrCat's AlphaNum pack, StrJoin over ranges, StrSplit's
// lazy splitter), which cannot bind directly -- these wrappers pin concrete
// signatures over the REAL absl entry points (the timetest pattern). absl::Cord
// itself binds head-on in binding.cpp; cord_str covers Cord -> std::string
// extraction (Cord's operator std::string is explicit and class-typed -- no
// numeric dunder).
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/cord.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

namespace strtest {

// StrCat: concrete arities over the AlphaNum pack (incl. number formatting).
inline std::string cat2(std::string_view a, std::string_view b) {
    return absl::StrCat(a, b);
}
inline std::string cat3(std::string_view a, std::string_view b, std::string_view c) {
    return absl::StrCat(a, b, c);
}
inline std::string cat_int(std::string_view a, std::int64_t n) {
    return absl::StrCat(a, n);
}

// StrJoin / StrSplit over the vector<string> caster.
inline std::string join(const std::vector<std::string>& parts, std::string_view sep) {
    return absl::StrJoin(parts, sep);
}
inline std::vector<std::string> split(std::string_view text, char sep) {
    return absl::StrSplit(text, sep);
}

// SimpleAtoi -> optional (exercises the optional caster on the miss path).
inline std::optional<std::int64_t> to_int(std::string_view s) {
    std::int64_t v;
    if (absl::SimpleAtoi(s, &v)) return v;
    return std::nullopt;
}

// ASCII utilities.
inline std::string upper(std::string_view s) { return absl::AsciiStrToUpper(s); }
inline std::string lower(std::string_view s) { return absl::AsciiStrToLower(s); }

// Cord -> std::string extraction.
inline std::string cord_str(const absl::Cord& c) { return std::string(c); }

}  // namespace strtest
