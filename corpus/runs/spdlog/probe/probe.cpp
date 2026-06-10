// Gate 1 probe: do spdlog's public headers compile under the p2996 toolchain at all?
// Header-only mode (no SPDLOG_COMPILED_LIB), in the same SPDLOG_USE_STD_FORMAT
// configuration the run binds. Includes the umbrella header plus the sink headers the
// run binds against (ostream sink for capture-based differential, stdout sink for the
// common case).
#define SPDLOG_USE_STD_FORMAT
#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

int main() {}
