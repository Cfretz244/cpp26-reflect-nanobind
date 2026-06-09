// Native C++ ground-truth oracle for the Abseil Duration/Time binding (Layer-1 differential).
// Drives the same operations through absl directly (via the shared timetest fixtures) and emits
// results; the bound module must reproduce them. Shared compiler + Abseil => divergence is the
// binding layer's.
#include "../binding/timetest.h"
#include "absl/time/time.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };

    using namespace timetest;
    add_i("s90_to_seconds", to_seconds(seconds(90)));
    add_i("s120_to_minutes", to_minutes(seconds(120)));
    add_b("min2_eq_s120", minutes(2) == seconds(120));
    add_i("sum_10_5", to_seconds(seconds(10) + seconds(5)));
    add_i("diff_100_40", to_seconds(seconds(100) - seconds(40)));
    add_b("s90_lt_h1", seconds(90) < hours(1));
    add_i("ms_1500_to_s", to_seconds(milliseconds(1500)));

    { std::ostringstream o; o << seconds(90); add_s("s90_str", o.str()); }   // "1m30s"

    add_i("unix_roundtrip", to_unix_seconds(from_unix_seconds(1000000000)));
    add_i("time_plus_dur", to_unix_seconds(from_unix_seconds(1000) + seconds(5)));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
