// Native C++ ground-truth oracle for the Abseil StatusOr binding (Layer-1 differential).
// Drives the SAME sotest fixture surface the Python test drives through the binding and
// observes it through the same members (ok/status/value), emitting JSON.
#include "../binding/sotest.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, long long v) {
        kv.emplace_back(k, std::to_string(v));
    };
    auto add_d = [&](const char* k, double v) {
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

    using namespace sotest;

    auto oi = ok_int(42);
    add_b("oi_ok", oi.ok());
    add_i("oi_value", get_int(oi));
    add_i("oi_code", static_cast<int>(oi.status().code()));   // kOk == 0

    auto ei = err_int(absl::StatusCode::kNotFound, "no int here");
    add_b("ei_ok", ei.ok());
    add_i("ei_raw", ei.status().raw_code());
    add_s("ei_msg", std::string(ei.status().message()));

    auto os = ok_str("hello statusor");
    add_b("os_ok", os.ok());
    add_s("os_value", get_str(os));

    auto es = err_str(absl::StatusCode::kInvalidArgument, "bad string");
    add_b("es_ok", es.ok());
    add_i("es_raw", es.status().raw_code());
    add_s("es_msg", std::string(es.status().message()));

    auto od = ok_dbl(2.5);
    add_b("od_ok", od.ok());
    add_d("od_value", get_dbl(od));

    auto ed = err_dbl(absl::StatusCode::kInternal, "bad double");
    add_b("ed_ok", ed.ok());
    add_i("ed_raw", ed.status().raw_code());

    // Default-constructed StatusOr<T> is the documented kUnknown error.
    absl::StatusOr<int> def;
    add_b("def_ok", def.ok());
    add_i("def_raw", def.status().raw_code());

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
