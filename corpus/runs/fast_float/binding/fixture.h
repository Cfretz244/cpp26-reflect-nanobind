// fixture.h -- thin Python-expressible wrappers over fast_float's free-function front.
//
// fast_float's API front is `from_chars(UC const* first, UC const* last, T& value, ...)`
// returning `from_chars_result_t<UC>` (a {const char* ptr, std::errc ec} pair). Pointer
// pairs and an out-parameter reference have no Python representation, so we wrap each
// concrete instantiation (double / float / int64 / uint64) as: take a std::string, call
// from_chars over [data, data+size), and return a small POD carrying the parsed value, the
// number of characters consumed (ptr - first), and an ok flag + the raw errc code. This is
// the ONLY way to exercise the parser from Python -- but the library's own bindable types
// (chars_format, from_chars_result_t<char>, parse_options_t<char>) are reflected head-on in
// binding.cpp, NOT wrapped here.
#pragma once

#include <fast_float/fast_float.h>

#include <cstdint>
#include <string>

namespace fixture {

using fast_float::chars_format;

// Result of a wrapped parse. Reflected as a plain aggregate-with-accessors:
// value (the parsed number, as double for the float instantiations / as the integer for the
// int ones), consumed (chars_consumed = ptr - first), ok (ec == std::errc()), ec (the raw
// std::errc as an int, 0 on success).
struct ParseDouble {
  double value;
  long consumed;
  bool ok;
  int ec;
};
struct ParseInt {
  long long value;
  long consumed;
  bool ok;
  int ec;
};
struct ParseUInt {
  unsigned long long value;
  long consumed;
  bool ok;
  int ec;
};

// --- double / float: from_chars with a chars_format ---
inline ParseDouble parse_double(std::string const &s,
                                chars_format fmt = chars_format::general) {
  double v = 0.0;
  auto r = fast_float::from_chars(s.data(), s.data() + s.size(), v, fmt);
  return {v, static_cast<long>(r.ptr - s.data()), r.ec == std::errc(),
          static_cast<int>(r.ec)};
}

inline ParseDouble parse_float(std::string const &s,
                               chars_format fmt = chars_format::general) {
  float v = 0.0f;
  auto r = fast_float::from_chars(s.data(), s.data() + s.size(), v, fmt);
  return {static_cast<double>(v), static_cast<long>(r.ptr - s.data()),
          r.ec == std::errc(), static_cast<int>(r.ec)};
}

// --- double via from_chars_advanced + a parse_options (custom decimal point etc.) ---
inline ParseDouble parse_double_opts(std::string const &s,
                                     fast_float::parse_options_t<char> opts) {
  double v = 0.0;
  auto r = fast_float::from_chars_advanced(s.data(), s.data() + s.size(), v, opts);
  return {v, static_cast<long>(r.ptr - s.data()), r.ec == std::errc(),
          static_cast<int>(r.ec)};
}

// --- integers: from_chars(..., int base) ---
inline ParseInt parse_int(std::string const &s, int base = 10) {
  long long v = 0;
  auto r = fast_float::from_chars(s.data(), s.data() + s.size(), v, base);
  return {v, static_cast<long>(r.ptr - s.data()), r.ec == std::errc(),
          static_cast<int>(r.ec)};
}

inline ParseUInt parse_uint(std::string const &s, int base = 10) {
  unsigned long long v = 0;
  auto r = fast_float::from_chars(s.data(), s.data() + s.size(), v, base);
  return {v, static_cast<long>(r.ptr - s.data()), r.ec == std::errc(),
          static_cast<int>(r.ec)};
}

} // namespace fixture
