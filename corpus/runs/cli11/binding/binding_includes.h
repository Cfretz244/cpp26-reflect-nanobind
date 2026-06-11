// Plain C++ includes for the CLI11 run -- parsed by BOTH the reflection
// toolchain (constexpr lane) and the production compiler (emit lane). No `^^`,
// no `[[=...]]`. The library header plus the config defines the binding needs;
// the -I include_root flag (the run's include_root) is on both stages' command
// line, so the angle-bracket form resolves on either compiler.
#pragma once

#include <CLI/CLI.hpp>
