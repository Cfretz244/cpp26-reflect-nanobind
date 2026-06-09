// Native C++ ground-truth oracle for the absl::Status binding (Layer-1 differential). Builds the
// same Status values the Python test drives through the binding and emits their observable
// properties. Shared compiler + shared Abseil => any divergence is the binding layer's.
#include "absl/status/status.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) { if (c == '\\' || c == '"') e += '\\'; e += c; }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    absl::Status ok = absl::OkStatus();
    add_b("ok_is_ok", ok.ok());
    add_i("ok_raw", ok.raw_code());

    absl::Status nf(absl::StatusCode::kNotFound, "thing missing");
    add_b("nf_is_ok", nf.ok());
    add_i("nf_raw", nf.raw_code());
    add_s("nf_msg", std::string(nf.message()));
    add_s("nf_str", nf.ToString());

    absl::Status inv(absl::StatusCode::kInvalidArgument, "bad arg");
    add_i("inv_raw", inv.raw_code());
    add_s("inv_msg", std::string(inv.message()));

    // enum ground-truth values
    add_i("code_kOk", static_cast<std::int64_t>(absl::StatusCode::kOk));
    add_i("code_kNotFound", static_cast<std::int64_t>(absl::StatusCode::kNotFound));
    add_i("code_kInvalidArgument", static_cast<std::int64_t>(absl::StatusCode::kInvalidArgument));
    add_i("code_kInternal", static_cast<std::int64_t>(absl::StatusCode::kInternal));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
