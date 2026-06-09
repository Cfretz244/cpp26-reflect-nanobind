// Native C++ ground-truth oracle for the Abseil InlinedVector binding (Layer-1 differential).
// Drives the SAME operations the Python test runs through the binding, using absl::InlinedVector
// directly, and emits the observed values as a JSON object on stdout. run_gates captures that as
// expected.json; test_bindings.py asserts the bound module reproduces every value. Native harness
// and bindings share one compiler + one Abseil, so any divergence is the binding layer's (and the
// small-buffer growth values are therefore directly comparable).
//
// NOTE: uses operator[] (not the bounds-checked at()) so this native TU does not reference
// absl::base_internal::ThrowStdOutOfRange — keeping the oracle link-free (build_native.sh links no
// extra absl sources). The bound module DOES expose at(); its throwing behavior is a Layer-3
// invariant, not part of the differential.
#include "absl/container/inlined_vector.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_d = [&](const char* k, double v) { kv.emplace_back(k, std::to_string(v)); };

    absl::InlinedVector<int, 4> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    add_i("iv_size", static_cast<std::int64_t>(v.size()));
    add_i("iv0", v[0]);
    add_i("iv1", v[1]);
    add_i("iv2", v[2]);
    add_i("iv_front", v.front());
    add_i("iv_back", v.back());
    add_i("iv_cap", static_cast<std::int64_t>(v.capacity()));  // == 4 (inline buffer, no heap)
    v.push_back(40);
    v.push_back(50);                                            // spills past the inline buffer of 4
    add_i("iv_size5", static_cast<std::int64_t>(v.size()));
    add_i("iv_cap5", static_cast<std::int64_t>(v.capacity()));  // heap-grown capacity
    v.pop_back();
    add_i("iv_pop_size", static_cast<std::int64_t>(v.size()));
    add_i("iv_pop_back", v.back());

    absl::InlinedVector<double, 2> d;
    d.push_back(1.5);
    d.push_back(2.5);
    add_i("dv_size", static_cast<std::int64_t>(d.size()));
    add_d("dv0", d[0]);
    add_d("dv1", d[1]);
    add_i("dv_cap", static_cast<std::int64_t>(d.capacity()));   // == 2 (inline)

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
