// Fixture namespace for the Abseil CRC32C run: thin inline wrappers over absl's
// free CRC functions (they live in ^^absl, which is not tractable to reflect
// whole; same pattern as abseil_time's timetest). The wrapped functions are the
// real Abseil entry points -- four resolve to compiled code in libabsl_merged.a.
// Shared by binding/binding.cpp and tests/oracle_native.cpp so both sides drive
// the identical surface.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "absl/crc/crc32c.h"

namespace crctest {

// Computation (header-inline in absl, dispatching into the compiled lib).
inline absl::crc32c_t compute(std::string_view buf) {
    return absl::ComputeCrc32c(buf);
}
inline absl::crc32c_t extend(absl::crc32c_t initial, std::string_view buf) {
    return absl::ExtendCrc32c(initial, buf);
}
inline absl::crc32c_t extend_by_zeroes(absl::crc32c_t initial, std::size_t length) {
    return absl::ExtendCrc32cByZeroes(initial, length);
}

// Arithmetic (compiled symbols in libabsl_merged.a).
inline absl::crc32c_t concat(absl::crc32c_t crc1, absl::crc32c_t crc2,
                             std::size_t crc2_length) {
    return absl::ConcatCrc32c(crc1, crc2, crc2_length);
}
inline absl::crc32c_t remove_prefix(absl::crc32c_t prefix_crc,
                                    absl::crc32c_t full_string_crc,
                                    std::size_t remaining_string_length) {
    return absl::RemoveCrc32cPrefix(prefix_crc, full_string_crc,
                                    remaining_string_length);
}
inline absl::crc32c_t remove_suffix(absl::crc32c_t full_string_crc,
                                    absl::crc32c_t suffix_crc,
                                    std::size_t suffix_length) {
    return absl::RemoveCrc32cSuffix(full_string_crc, suffix_crc, suffix_length);
}

// crc32c_t's operator== is a hidden friend (not an enumerable namespace member),
// so equality is surfaced through a wrapper; value() mirrors the explicit
// uint32_t conversion for cross-checking the bound __int__.
inline bool eq(absl::crc32c_t a, absl::crc32c_t b) { return a == b; }
inline std::uint32_t value(absl::crc32c_t c) { return static_cast<std::uint32_t>(c); }

}  // namespace crctest
