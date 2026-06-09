// Native C++ ground-truth oracle for the Abseil CRC32C binding (Layer-1 differential).
// Drives the SAME crctest fixture surface the Python test drives through the binding,
// using native Abseil, and emits each result as JSON. Shared compiler + shared Abseil
// archive => any divergence is the binding layer's.
#include "../binding/crctest.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_u = [&](const char* k, std::uint64_t v) {
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

    using namespace crctest;
    const std::string hello = "hello ", world = "world", both = "hello world";

    // Plain computation + the strong-typedef value channel.
    absl::crc32c_t c_both = compute(both);
    add_u("crc_hello_world", value(c_both));
    add_u("crc_empty", value(compute("")));

    // Streamed form ("%08x" hex) -- the bound __str__'s channel.
    { std::ostringstream o; o << c_both; add_s("crc_hello_world_str", o.str()); }

    // extend(compute(prefix), suffix) == compute(prefix+suffix)
    add_u("crc_extended", value(extend(compute(hello), world)));
    add_b("extend_matches_full", eq(extend(compute(hello), world), c_both));

    // extend_by_zeroes: appending 5 NUL bytes.
    add_u("crc_zeroes", value(extend_by_zeroes(compute(hello), 5)));

    // concat(crc1, crc2, len2) == compute(buf1+buf2)
    add_u("crc_concat", value(concat(compute(hello), compute(world), world.size())));
    add_b("concat_matches_full",
          eq(concat(compute(hello), compute(world), world.size()), c_both));

    // remove_prefix / remove_suffix invert the concatenation.
    add_u("crc_rm_prefix",
          value(remove_prefix(compute(hello), c_both, world.size())));
    add_b("rm_prefix_matches",
          eq(remove_prefix(compute(hello), c_both, world.size()), compute(world)));
    add_u("crc_rm_suffix",
          value(remove_suffix(c_both, compute(world), world.size())));
    add_b("rm_suffix_matches",
          eq(remove_suffix(c_both, compute(world), world.size()), compute(hello)));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
