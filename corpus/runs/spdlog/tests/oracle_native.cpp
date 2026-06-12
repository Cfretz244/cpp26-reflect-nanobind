// Native C++ ground-truth oracle for the spdlog binding (Layer-1 differential). Drives the
// EXACT sequence the Python test drives through the bound module -- same logger name, same
// pattern, same messages, same level filtering, same clone -- via the shared logtest
// CaptureSink fixture, and emits every observable as JSON. Shared compiler + shared
// (header-only) spdlog => any divergence is the binding layer's.
#include "../binding/logtest.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/null_sink.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') { e += "\\n"; }
            else { e += c; }
        }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    using spdlog::level::level_enum;

    // --- the shared logging scenario (mirrored verbatim in test_bindings.py) ---
    logtest::CaptureSink cap;
    spdlog::logger lg("pylog", cap.sink());
    lg.set_pattern("[%n] [%l] %v", spdlog::pattern_time_type::local);
    lg.set_level(level_enum::debug);
    lg.log(level_enum::info, std::string_view("hello world"));
    lg.log(level_enum::trace, std::string_view("filtered out"));   // below debug => dropped
    lg.log(level_enum::err, std::string_view("boom"));
    lg.flush();
    add_s("cap_text", cap.text());

    add_s("logger_name", lg.name());
    add_i("logger_level", static_cast<std::int64_t>(lg.level()));
    add_i("flush_level_default", static_cast<std::int64_t>(lg.flush_level()));
    add_b("should_log_trace", lg.should_log(level_enum::trace));
    add_b("should_log_debug", lg.should_log(level_enum::debug));
    add_b("should_log_err", lg.should_log(level_enum::err));
    add_i("n_sinks", static_cast<std::int64_t>(lg.sinks().size()));

    // clone shares the sink vector; its output lands in the same capture.
    auto cl = lg.clone("worker");
    cap.clear();
    cl->log(level_enum::warn, std::string_view("from clone"));
    cl->flush();
    add_s("clone_name", cl->name());
    add_s("clone_text", cap.text());

    // --- log-level enum + name tables ---
    add_i("lvl_trace", static_cast<std::int64_t>(level_enum::trace));
    add_i("lvl_debug", static_cast<std::int64_t>(level_enum::debug));
    add_i("lvl_info", static_cast<std::int64_t>(level_enum::info));
    add_i("lvl_warn", static_cast<std::int64_t>(level_enum::warn));
    add_i("lvl_err", static_cast<std::int64_t>(level_enum::err));
    add_i("lvl_critical", static_cast<std::int64_t>(level_enum::critical));
    add_i("lvl_off", static_cast<std::int64_t>(level_enum::off));
    add_s("name_trace", std::string(spdlog::level::to_string_view(level_enum::trace)));
    add_s("name_info", std::string(spdlog::level::to_string_view(level_enum::info)));
    add_s("name_warn", std::string(spdlog::level::to_string_view(level_enum::warn)));
    add_s("name_err", std::string(spdlog::level::to_string_view(level_enum::err)));
    add_s("short_critical", spdlog::level::to_short_c_str(level_enum::critical));
    add_i("from_str_warning", static_cast<std::int64_t>(spdlog::level::from_str("warning")));
    add_i("from_str_trace", static_cast<std::int64_t>(spdlog::level::from_str("trace")));

    // --- concrete sink defaults (stdout_sink_mt: sink base surface) ---
    spdlog::sinks::stdout_sink_mt s;
    add_i("sink_default_level", static_cast<std::int64_t>(s.level()));
    add_b("sink_should_info", s.should_log(level_enum::info));
    s.set_level(level_enum::err);
    add_b("sink_should_info_after", s.should_log(level_enum::info));

    // --- the derived_from_-matched sink family ---
    // null_sink: a logger over it runs the full pipeline with no observable output.
    auto ns = std::make_shared<spdlog::sinks::null_sink_mt>();
    add_i("null_default_level", static_cast<std::int64_t>(ns->level()));
    spdlog::logger nlg("nulllog", ns);
    nlg.log(level_enum::info, std::string_view("swallowed"));
    nlg.flush();
    add_i("null_n_sinks", static_cast<std::int64_t>(nlg.sinks().size()));
    // ansicolor: color_mode::always/never makes should_color() deterministic
    // (no tty dependence); set_color_mode flips it.
    spdlog::sinks::ansicolor_stdout_sink_mt ac(spdlog::color_mode::always);
    add_b("ansi_always", ac.should_color());
    ac.set_color_mode(spdlog::color_mode::never);
    add_b("ansi_never", ac.should_color());
    // stderr sink: same base surface through the second stdout_sinks template.
    spdlog::sinks::stderr_sink_mt es;
    add_b("stderr_should_info", es.should_log(level_enum::info));

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
