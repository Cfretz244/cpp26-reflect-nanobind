// Gate 1 probe for toml++ v3.4.0. Header-only; compiled with TOML_EXCEPTIONS=0 so the
// real toml::parse_result class (a node-or-error variant) is available to bind head-on,
// rather than the `using parse_result = table;` alias that the exceptions-on build emits.
// TOML_IMPLEMENTATION pulls in the .inl translation units (parse(), formatters, node.inl)
// so the parse/serialize free functions actually have definitions in this TU.
#define TOML_EXCEPTIONS 0
#define TOML_IMPLEMENTATION
#include <toml++/toml.hpp>

int main() {}
