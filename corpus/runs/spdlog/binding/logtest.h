// logtest -- capture fixture for the spdlog run, shared by binding.cpp and the native oracle.
//
// Python needs to OBSERVE what a logger writes; spdlog's observable sink for that is
// ostream_sink, whose ctor takes std::ostream& (no caster, and we don't want
// std::basic_ostream in the bind set). CaptureSink owns an ostringstream plus an
// ostream_sink_mt writing into it and exposes only binder-friendly signatures:
// sink() -> shared_ptr<sinks::sink> (feeds logger's real sink_ptr ctor) and text().
//
// This header must be included before any other spdlog include: it selects
// SPDLOG_USE_STD_FORMAT (a first-class spdlog config), making string_view_t =
// std::string_view and format_string_t = std::format_string -- the types the nanobind
// STL casters actually cover (default bundled-fmt mode uses fmt::basic_string_view).
#pragma once

#ifndef SPDLOG_USE_STD_FORMAT
#define SPDLOG_USE_STD_FORMAT
#endif

#include <spdlog/logger.h>
#include <spdlog/sinks/ostream_sink.h>

#include <memory>
#include <sstream>
#include <string>

namespace logtest {

class CaptureSink {
public:
    CaptureSink()
        : oss_(std::make_shared<std::ostringstream>()),
          sink_(std::make_shared<spdlog::sinks::ostream_sink_mt>(*oss_, /*force_flush=*/true)) {}

    std::shared_ptr<spdlog::sinks::sink> sink() const { return sink_; }
    std::string text() const { return oss_->str(); }
    void clear() { oss_->str(std::string()); }

private:
    std::shared_ptr<std::ostringstream> oss_;
    std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink_;
};

}  // namespace logtest
