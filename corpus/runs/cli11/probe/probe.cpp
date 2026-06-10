// Gate 1 probe: do CLI11's public headers compile under the p2996 toolchain at all?
// CLI11 is header-only; <CLI/CLI.hpp> is the umbrella include that pulls in App, Option,
// Error, Validators, etc.
#include <CLI/CLI.hpp>

int main() {}
