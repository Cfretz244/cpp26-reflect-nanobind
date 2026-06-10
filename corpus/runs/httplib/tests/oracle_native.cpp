// Native C++ ground-truth oracle for the cpp-httplib binding (Layer-1 differential).
//
// Spins the EXACT same httptest::EchoServer fixture the binding uses (a real httplib::Server
// on a background thread, bound to 127.0.0.1 at an ephemeral port) and drives the SAME
// sequence of REAL httplib::Client calls the Python test drives -- Get/Post/Put/Delete with
// the identical paths/bodies/content-types/progress callback -- then emits every observable
// (status codes, response bodies, content types, echoed headers, the Error enum on a
// closed-port connect) as one JSON object on stdout. Shared compiler + shared httplib.h =>
// any divergence vs the Python-driven bound client is the binding layer's.
#include "../binding/httptest.h"

#include "httplib.h"

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
            else if (c == '\t') { e += "\\t"; }
            else { e += c; }
        }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    httptest::EchoServer srv;
    int port = srv.start();
    add_b("port_positive", port > 0);

    httplib::Client cli("127.0.0.1", port);
    add_b("client_is_valid", cli.is_valid());

    // always-continue progress callback (passed explicitly on both sides since the
    // default nullptr is a C++26-unbindable default value).
    httplib::DownloadProgress dprog = [](size_t, size_t) { return true; };
    httplib::UploadProgress uprog = [](size_t, size_t) { return true; };

    // --- GET /echo: status, body (method+path echo), echoed method header ---
    {
        auto r = cli.Get("/echo", dprog);
        add_b("get_ok", static_cast<bool>(r));
        add_i("get_status", r->status);
        add_s("get_body", r->body);
        add_s("get_ct", r->get_header_value("Content-Type", "", 0));
        add_s("get_echo_method", r->get_header_value("X-Echo-Method", "", 0));
        add_s("get_echo_path", r->get_header_value("X-Echo-Path", "", 0));
        add_i("get_error", static_cast<std::int64_t>(r.error()));
    }

    // --- GET /missing -> 404 (no route) ---
    {
        auto r = cli.Get("/missing", dprog);
        add_b("missing_ok", static_cast<bool>(r));
        add_i("missing_status", r->status);
    }

    // --- GET /teapot -> 418 with body ---
    {
        auto r = cli.Get("/teapot", dprog);
        add_i("teapot_status", r->status);
        add_s("teapot_body", r->body);
    }

    // --- POST /echo: 201, body echoed, content-type mirrored, body-len header ---
    {
        std::string body = "hello world";
        auto r = cli.Post("/echo", body, "text/plain", uprog);
        add_b("post_ok", static_cast<bool>(r));
        add_i("post_status", r->status);
        add_s("post_body", r->body);
        add_s("post_ct", r->get_header_value("Content-Type", "", 0));
        add_s("post_body_len", r->get_header_value("X-Body-Len", "", 0));
    }

    // --- POST with a different body/content-type ---
    {
        std::string body = "{\"k\":42}";
        auto r = cli.Post("/echo", body, "application/json", uprog);
        add_i("post2_status", r->status);
        add_s("post2_body", r->body);
        add_s("post2_ct", r->get_header_value("Content-Type", "", 0));
    }

    // --- PUT /echo: body echoed with PUT: prefix ---
    {
        auto r = cli.Put("/echo", "payload", "text/plain", uprog);
        add_i("put_status", r->status);
        add_s("put_body", r->body);
    }

    // --- DELETE /echo -> 204 ---
    {
        auto r = cli.Delete("/echo", dprog);
        add_b("delete_ok", static_cast<bool>(r));
        add_i("delete_status", r->status);
    }

    // --- enum reference values ---
    add_i("err_success_value", static_cast<std::int64_t>(httplib::Error::Success));
    add_i("err_connection_value", static_cast<std::int64_t>(httplib::Error::Connection));
    add_i("status_ok_value", static_cast<std::int64_t>(httplib::StatusCode::OK_200));
    add_i("status_created_value", static_cast<std::int64_t>(httplib::StatusCode::Created_201));
    add_i("status_nocontent_value", static_cast<std::int64_t>(httplib::StatusCode::NoContent_204));
    add_i("status_notfound_value", static_cast<std::int64_t>(httplib::StatusCode::NotFound_404));

    srv.stop();

    // --- error path: connect to a closed port -> Result falsy, Error::Connection ---
    {
        httplib::Client dead("127.0.0.1", 1); // port 1 is not listening
        auto r = dead.Get("/x", dprog);
        add_b("closed_ok", static_cast<bool>(r));
        add_i("closed_error", static_cast<std::int64_t>(r.error()));
    }

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
