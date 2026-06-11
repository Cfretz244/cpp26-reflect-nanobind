// Plain C++ includes for the spdlog run (both compilers). logtest.h comes
// first: it selects SPDLOG_USE_STD_FORMAT before any spdlog header. No ^^,
// no annotations.
#pragma once
#include "logtest.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
