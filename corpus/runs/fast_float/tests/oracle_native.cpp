// Native C++ ground-truth oracle for the fast_float binding (Layer-1 differential).
// Drives the EXACT sequence the Python test drives through the bound module -- same input
// strings, same chars_format values, same parse_options -- straight through native
// fast_float, and emits every observable (parsed value, chars consumed, ok flag, errc) as
// JSON. Shared compiler + shared (header-only) fast_float => any divergence is the binding
// layer's. The fixture header is shared so the wrapping logic is byte-identical.
#include "../binding/fixture.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using fast_float::chars_format;

int main() {
  std::vector<std::pair<std::string, std::string>> kv;

  auto num = [](double v) -> std::string {
    // round-trippable repr that Python's json + float compare exactly; use 17 sig digits.
    if (std::isnan(v)) return "\"nan\"";
    if (std::isinf(v)) return v < 0 ? "\"-inf\"" : "\"inf\"";
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.17g", v);
    return std::string(buf);
  };
  auto add_d = [&](std::string const &k, fixture::ParseDouble const &r) {
    kv.emplace_back(k + "_value", num(r.value));
    kv.emplace_back(k + "_consumed", std::to_string(r.consumed));
    kv.emplace_back(k + "_ok", r.ok ? "true" : "false");
    kv.emplace_back(k + "_ec", std::to_string(r.ec));
  };
  auto add_i = [&](std::string const &k, fixture::ParseInt const &r) {
    kv.emplace_back(k + "_value", std::to_string(r.value));
    kv.emplace_back(k + "_consumed", std::to_string(r.consumed));
    kv.emplace_back(k + "_ok", r.ok ? "true" : "false");
    kv.emplace_back(k + "_ec", std::to_string(r.ec));
  };
  auto add_u = [&](std::string const &k, fixture::ParseUInt const &r) {
    kv.emplace_back(k + "_value", std::to_string(r.value));
    kv.emplace_back(k + "_consumed", std::to_string(r.consumed));
    kv.emplace_back(k + "_ok", r.ok ? "true" : "false");
    kv.emplace_back(k + "_ec", std::to_string(r.ec));
  };
  auto add_int_raw = [&](std::string const &k, long v) {
    kv.emplace_back(k, std::to_string(v));
  };

  chars_format const G = chars_format::general;
  chars_format const H = chars_format::hex;
  chars_format const S = chars_format::scientific;
  chars_format const F = chars_format::fixed;

  // --- double: ordinary, trailing, error, scientific, tricky rounding ---
  add_d("d_pi", fixture::parse_double("3.14159 rest", G));
  add_d("d_trail", fixture::parse_double("2.5xyz", G));
  add_d("d_err", fixture::parse_double("nope", G));
  add_d("d_empty", fixture::parse_double("", G));
  add_d("d_sci", fixture::parse_double("1.5e3", G));
  add_d("d_neg", fixture::parse_double("-0.0", G));
  // subnormal (smallest positive double) + round-to-even tie
  add_d("d_subnormal", fixture::parse_double("5e-324", G));
  add_d("d_tie", fixture::parse_double("1.0000000000000002", G));
  add_d("d_huge", fixture::parse_double("1e400", G));   // overflow -> inf, ec set
  // inf / nan literals
  add_d("d_inf", fixture::parse_double("inf", G));
  add_d("d_nan", fixture::parse_double("nan", G));

  // --- format gating (the chars_format theme) ---
  add_d("h_prefix", fixture::parse_double("0x1.8p1", H));   // 0x not consumed in hex mode
  add_d("h_noprefix", fixture::parse_double("1.8p1", H));
  add_d("s_only_ok", fixture::parse_double("1e10", S));     // sci accepted
  add_d("s_reject_fixed", fixture::parse_double("12.5", S)); // fixed rejected under scientific
  add_d("f_only_ok", fixture::parse_double("12.5", F));     // fixed accepted
  add_d("f_reject_sci", fixture::parse_double("1e5", F));   // sci rejected under fixed

  // --- float (single precision) ---
  add_d("f32_third", fixture::parse_float("0.1", G));
  add_d("f32_big", fixture::parse_float("3.4e38", G));

  // --- options: custom decimal point + base via from_chars_advanced ---
  fast_float::parse_options_t<char> comma_opts(G, ',', 10);
  add_d("opt_comma", fixture::parse_double_opts("3,5", comma_opts));
  add_d("opt_comma_dotfail", fixture::parse_double_opts("3.5", comma_opts));

  // --- integers ---
  add_i("i_pos", fixture::parse_int("12345", 10));
  add_i("i_neg", fixture::parse_int("-42xyz", 10));
  add_i("i_err", fixture::parse_int("abc", 10));
  add_i("i_hex", fixture::parse_int("ff", 16));
  add_i("i_bin", fixture::parse_int("1010", 2));
  add_u("u_dec", fixture::parse_uint("255 ", 16));   // hex 0x255 = 597
  add_u("u_big", fixture::parse_uint("18446744073709551615", 10)); // UINT64_MAX
  add_u("u_overflow", fixture::parse_uint("18446744073709551616", 10)); // overflow

  // --- chars_format enum underlying values (differential on the enum itself) ---
  add_int_raw("cf_scientific", static_cast<long>(chars_format::scientific));
  add_int_raw("cf_fixed", static_cast<long>(chars_format::fixed));
  add_int_raw("cf_hex", static_cast<long>(chars_format::hex));
  add_int_raw("cf_no_infnan", static_cast<long>(chars_format::no_infnan));
  add_int_raw("cf_general", static_cast<long>(chars_format::general));
  add_int_raw("cf_json", static_cast<long>(chars_format::json));
  add_int_raw("cf_json_or_infnan", static_cast<long>(chars_format::json_or_infnan));
  add_int_raw("cf_fortran", static_cast<long>(chars_format::fortran));
  add_int_raw("cf_allow_leading_plus", static_cast<long>(chars_format::allow_leading_plus));
  add_int_raw("cf_skip_white_space", static_cast<long>(chars_format::skip_white_space));

  std::cout << "{";
  for (size_t i = 0; i < kv.size(); ++i) {
    if (i) std::cout << ",";
    std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
  }
  std::cout << "}" << std::endl;
  return 0;
}
