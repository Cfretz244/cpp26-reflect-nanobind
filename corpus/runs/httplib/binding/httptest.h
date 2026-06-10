// httptest -- loopback server fixture for the cpp-httplib run.
//
// cpp-httplib's Server runs a blocking accept loop (listen/listen_after_bind) and registers
// route handlers as std::function values. NEITHER is Python-expressible: the accept loop must
// run on a background thread, and the echo handlers run ON that thread inside the library --
// they cannot be Python callables driven from the test. So this fixture owns BOTH: it spins a
// real httplib::Server on a std::thread bound to 127.0.0.1 at an ephemeral port, registers
// handlers in C++ that echo the request's method / path / body / a chosen header back into the
// response (status, body, content-type, and echo headers), and exposes only start()->port and
// stop(). EVERYTHING the test observes -- status codes, response bodies, content types, header
// round-trips, the Error enum on a closed-port connect -- is produced by driving the REAL bound
// httplib::Client from Python against this real server. The fixture adds no behavior of its own
// beyond thread lifecycle + the C++-side handler registration httplib requires.
#pragma once

#include "httplib.h"

#include <atomic>
#include <string>
#include <thread>

namespace httptest {

// A loopback HTTP server on a background thread. Handlers registered in C++ echo request
// data back so the Python-driven client sees deterministic, request-derived responses.
class EchoServer {
public:
  EchoServer() {
    // GET /echo  -> 200, body echoes method+path, content-type text/plain,
    //               and reflects the X-Probe request header into X-Echo-Probe.
    svr_.Get("/echo", [](const httplib::Request &req, httplib::Response &res) {
      std::string probe = req.get_header_value("X-Probe");
      res.set_header("X-Echo-Method", req.method);
      res.set_header("X-Echo-Path", req.path);
      res.set_header("X-Echo-Probe", probe);
      res.set_content(req.method + " " + req.path, "text/plain");
    });

    // GET /param?q=... -> reflect the query param value in the body.
    svr_.Get("/param", [](const httplib::Request &req, httplib::Response &res) {
      res.set_content(req.get_param_value("q"), "text/plain");
    });

    // POST /echo -> 201, body echoes the posted body, content-type mirrors the
    //               request's Content-Type header; reflect the body length.
    svr_.Post("/echo", [](const httplib::Request &req, httplib::Response &res) {
      res.status = httplib::StatusCode::Created_201;
      res.set_header("X-Echo-Method", req.method);
      res.set_header("X-Body-Len", std::to_string(req.body.size()));
      std::string ct = req.get_header_value("Content-Type");
      res.set_content(req.body, ct.empty() ? "application/octet-stream" : ct);
    });

    // PUT /echo -> 200, echoes body uppercased length info.
    svr_.Put("/echo", [](const httplib::Request &req, httplib::Response &res) {
      res.set_content("PUT:" + req.body, "text/plain");
    });

    // DELETE /echo -> 204 no content.
    svr_.Delete("/echo", [](const httplib::Request &, httplib::Response &res) {
      res.status = httplib::StatusCode::NoContent_204;
    });

    // GET /status/<n> via a fixed route -> a chosen non-200 status with a body.
    svr_.Get("/teapot", [](const httplib::Request &, httplib::Response &res) {
      res.status = 418; // I'm a teapot
      res.set_content("teapot", "text/plain");
    });

    // (no handler for /missing -> server returns 404)
  }

  // Start the accept loop on a background thread; returns the bound ephemeral port.
  // Blocks until the server is actually accepting connections (wait_until_ready).
  int start() {
    // bind_to_any_port binds 127.0.0.1 to an OS-picked ephemeral port and returns it.
    port_ = svr_.bind_to_any_port("127.0.0.1");
    thread_ = std::thread([this] { svr_.listen_after_bind(); });
    svr_.wait_until_ready();
    return port_;
  }

  // Stop the accept loop and join the background thread.
  void stop() {
    svr_.stop();
    if (thread_.joinable()) thread_.join();
  }

  int port() const { return port_; }

  ~EchoServer() { stop(); }

private:
  httplib::Server svr_;
  std::thread thread_;
  int port_ = 0;
};

} // namespace httptest
