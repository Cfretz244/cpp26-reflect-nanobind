// Native ground-truth oracle for the date binding (Layer-1 differential). Drives the
// EXACT calendrical scenario the Python test drives through the bound module -- the same
// year/month/day construction via operator/, the same comparisons, conversions, ok()
// checks, weekday computation, sys_days serial round-trip, and formatted output -- and
// emits every observable as ONE JSON object. Shared compiler + shared header-only date =>
// any divergence is the binding layer's.
#include "../binding/datefix.h"

#include <date/date.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace date;

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto esc = [](const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') { e += "\\n"; }
            else e += c;
        }
        return e + "\"";
    };
    auto add_s = [&](const char* k, const std::string& v) { kv.emplace_back(k, esc(v)); };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto str_of = [](auto const& x) { std::ostringstream os; os << x; return os.str(); };

    // --- value-type construction + accessors + operator<< (__str__) ---
    year  y{2024};
    month mo{2};
    day   d{29};
    add_i("year_int", static_cast<int>(y));
    add_s("year_str", str_of(y));
    add_b("year_leap", y.is_leap());
    add_b("year_ok", y.ok());
    add_i("month_int", static_cast<unsigned>(mo));
    add_s("month_str", str_of(mo));
    add_i("day_int", static_cast<unsigned>(d));
    add_s("day_str", str_of(d));

    // --- operator/ calendar composition (the date DSL) ---
    year_month ym = y / mo;
    add_s("ym_str", str_of(ym));
    add_i("ym_year", static_cast<int>(ym.year()));
    add_i("ym_month", static_cast<unsigned>(ym.month()));

    month_day md = month{7} / day{4};
    add_s("md_str", str_of(md));
    add_i("md_month", static_cast<unsigned>(md.month()));
    add_i("md_day", static_cast<unsigned>(md.day()));

    year_month_day ymd = y / mo / d;        // 2024-02-29
    add_s("ymd_str", str_of(ymd));
    add_i("ymd_year", static_cast<int>(ymd.year()));
    add_i("ymd_month", static_cast<unsigned>(ymd.month()));
    add_i("ymd_day", static_cast<unsigned>(ymd.day()));
    add_b("ymd_ok", ymd.ok());

    // --- ok() validity checks ---
    add_b("month13_ok", month{13}.ok());
    add_b("day32_ok", day{32}.ok());
    add_b("feb29_2023_ok", (year{2023} / month{2} / day{29}).ok());

    // --- comparisons (rich-comparison dunders) ---
    add_b("y2024_lt_y2020", (year{2024} < year{2020}));
    add_b("y2024_gt_y2020", (year{2024} > year{2020}));
    add_b("y2024_eq_y2024", (year{2024} == year{2024}));
    add_b("feb_lt_mar", (month{2} < month{3}));
    add_b("ymd_lt", ((year{2024}/month{1}/day{1}) < (year{2024}/month{2}/day{1})));
    add_b("ymd_eq", ((year{2024}/month{2}/day{29}) == year_month_day{sys_days{days{19782}}}));

    // --- sys_days serial round-trip (the fixture bridge) ---
    long serial = datefix::ymd_to_serial(ymd);
    add_i("ymd_serial", serial);
    add_s("serial_back_str", str_of(datefix::serial_to_ymd(serial)));

    // serial of the epoch and a known reference (1970-01-01 == 0).
    add_i("epoch_serial", datefix::ymd_to_serial(year{1970}/month{1}/day{1}));
    add_i("y2000_serial", datefix::ymd_to_serial(year{2000}/month{1}/day{1}));

    // --- weekday computation from a date (weekday(sys_days)) ---
    weekday wd = datefix::weekday_of(ymd);  // 2024-02-29 is a Thursday
    add_s("wd_str", str_of(wd));
    add_i("wd_c_encoding", wd.c_encoding());
    add_i("wd_iso_encoding", wd.iso_encoding());
    // a Sunday: 2024-03-03
    weekday wd_sun = datefix::weekday_of(year{2024}/month{3}/day{3});
    add_s("wd_sun_str", str_of(wd_sun));
    add_i("wd_sun_c", wd_sun.c_encoding());
    add_i("wd_sun_iso", wd_sun.iso_encoding());

    // --- formatted output (date::format via the fixture) ---
    add_s("fmt_iso", datefix::format_ymd("%Y-%m-%d", ymd));
    add_s("fmt_long", datefix::format_ymd("%B %d, %Y", ymd));
    add_s("fmt_wd_full", datefix::format_weekday("%A", wd));
    add_s("fmt_wd_abbr", datefix::format_weekday("%a", wd));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
