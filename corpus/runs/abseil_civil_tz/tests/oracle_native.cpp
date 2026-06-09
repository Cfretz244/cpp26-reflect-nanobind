// Native C++ ground-truth oracle for the Abseil civil-time/TimeZone binding
// (Layer-1 differential). Drives the SAME surface the Python test drives through
// the binding (civil ctors/accessors/+=, TimeZone::At -> CivilInfo, tztest
// conversions), using native Abseil, and emits each result as JSON. UTC/fixed
// zones only (tzdata-independent).
#include "../binding/tztest.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, long long v) {
        kv.emplace_back(k, std::to_string(v));
    };
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };
    auto add_b = [&](const char* k, bool v) {
        kv.emplace_back(k, v ? "true" : "false");
    };

    using namespace tztest;

    // Civil construction + accessors.
    absl::CivilDay d(2015, 6, 13);
    add_i("d_year", d.year()); add_i("d_month", d.month()); add_i("d_day", d.day());
    add_i("d_weekday", weekday_index(d));                  // 2015-06-13 is a Saturday

    // Field normalization (the signature civil-time behavior).
    absl::CivilDay norm(2016, 2, 30);
    add_i("norm_year", norm.year()); add_i("norm_month", norm.month());
    add_i("norm_day", norm.day());

    // Arithmetic via member += (the bound __iadd__).
    absl::CivilDay adv(2015, 6, 13);
    adv += 30;
    add_i("adv_year", adv.year()); add_i("adv_month", adv.month());
    add_i("adv_day", adv.day());

    // TimeZone::At(Time) -> CivilInfo, at the epoch, in UTC and at +01:00.
    absl::TimeZone z = utc();
    add_s("utc_name", z.name());
    auto ci = z.At(from_unix_seconds(0));
    add_i("ci_year", ci.cs.year()); add_i("ci_month", ci.cs.month());
    add_i("ci_day", ci.cs.day());  add_i("ci_hour", ci.cs.hour());
    add_i("ci_offset", ci.offset);
    add_b("ci_dst", ci.is_dst);
    add_s("ci_abbr", ci.zone_abbr);

    absl::TimeZone plus1 = fixed(3600);
    auto ci1 = plus1.At(from_unix_seconds(0));
    add_i("ci1_hour", ci1.cs.hour());
    add_i("ci1_offset", ci1.offset);

    // Civil -> absolute round-trip and formatting.
    absl::CivilSecond cs(2000, 1, 2, 3, 4, 5);
    add_i("rt_unix", to_unix_seconds(from_civil(cs, z)));
    add_s("fmt_epoch", format_sec(from_unix_seconds(0), z));
    add_s("fmt_epoch_plus1", format_sec(from_unix_seconds(0), plus1));

    // CivilYear truncation: below-year fields are dropped at construction.
    absl::CivilYear y(2015, 6, 13);
    add_i("y_year", y.year()); add_i("y_month", y.month());

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
