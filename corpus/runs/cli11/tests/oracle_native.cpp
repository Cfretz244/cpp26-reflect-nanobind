// Native C++ ground-truth oracle for the CLI11 binding (Layer-1 differential). Drives the
// EXACT same scenarios the Python test drives through the bound module -- same option/flag
// specs, the same reversed argv vectors (CLI11's parse(vector<string>&) expects a reversed
// vector), the same success and error paths -- and emits every observable as one JSON object.
// Shared compiler + shared (header-only) CLI11 => any divergence is the binding layer's.
#include <CLI/CLI.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
std::vector<std::pair<std::string, std::string>> kv;
void add_s(const char* k, const std::string& v) {
    std::string e = "\"";
    for (char c : v) {
        if (c == '\\' || c == '"') { e += '\\'; e += c; }
        else if (c == '\n') { e += "\\n"; }
        else { e += c; }
    }
    kv.emplace_back(k, e + "\"");
}
void add_i(const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); }

// CLI11's parse(vector<string>&) expects the reversed forward command line.
std::vector<std::string> rev(std::vector<std::string> a) {
    std::reverse(a.begin(), a.end());
    return a;
}
}  // namespace

int main() {
    // --- scenario 1: a successful parse with option + repeated flags ---
    {
        CLI::App app{"demo", "prog"};
        auto* file = app.add_option("--file,-f");
        auto* verbose = app.add_flag("--verbose,-v");
        auto* cnt = app.add_flag("--count,-c");
        auto args = rev({"--file", "x.txt", "-v", "-c", "-c"});
        app.parse(args);
        add_i("file_count", static_cast<std::int64_t>(file->count()));
        add_s("file_result0", file->count() ? file->results()[0] : std::string{});
        add_i("verbose_count", static_cast<std::int64_t>(verbose->count()));
        add_i("cnt_count", static_cast<std::int64_t>(cnt->count()));
        add_i("app_count_file", static_cast<std::int64_t>(app.count("--file")));
        add_s("file_single_name", file->get_single_name());
        add_i("file_snames", static_cast<std::int64_t>(file->get_snames().size()));
        add_i("file_lnames", static_cast<std::int64_t>(file->get_lnames().size()));
        add_s("app_description", app.get_description());
    }

    // --- scenario 2: multi-value option collecting several results ---
    {
        CLI::App app{"multi", "prog"};
        auto* nums = app.add_option("--num,-n");
        nums->expected(3)->take_all();  // accept exactly three values
        auto args = rev({"--num", "1", "2", "3"});
        app.parse(args);
        add_i("num_count", static_cast<std::int64_t>(nums->count()));
        std::string joined;
        for (auto& r : nums->results()) { if (!joined.empty()) joined += ","; joined += r; }
        add_s("num_results", joined);
    }

    // --- scenario 3 (error path): a required option missing -> RequiredError ---
    {
        CLI::App app{"req", "prog"};
        app.add_option("--req")->required(true);
        std::string what, kind;
        try {
            std::vector<std::string> empty;
            app.parse(empty);
            kind = "none";
        } catch (const CLI::RequiredError& e) {
            kind = "RequiredError";
            what = e.what();
        } catch (const CLI::ParseError& e) {
            kind = "ParseError";
            what = e.what();
        }
        add_s("req_error_kind", kind);
        add_s("req_error_what", what);
    }

    // --- scenario 4 (error path): an unexpected extra argument -> ExtrasError ---
    {
        CLI::App app{"extras", "prog"};
        app.add_flag("--known");
        std::string what, kind;
        try {
            auto args = rev({"--unknown"});
            app.parse(args);
            kind = "none";
        } catch (const CLI::ExtrasError& e) {
            kind = "ExtrasError";
            what = e.what();
        } catch (const CLI::ParseError& e) {
            kind = "ParseError";
            what = e.what();
        }
        add_s("extras_error_kind", kind);
        add_s("extras_error_what", what);
    }

    std::cout << "{";
    for (std::size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
