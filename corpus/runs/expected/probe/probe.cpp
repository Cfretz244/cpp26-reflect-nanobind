// Gate 1 probe: does tl::expected's single public header compile under the p2996
// toolchain at all? The library is header-only with exactly one public header.
#include "tl/expected.hpp"

int main() {}
