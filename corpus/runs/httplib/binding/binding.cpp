// Single-stage bindings for cpp-httplib v0.47.0 (Tier 4: a real loopback HTTP
// client/server, value types, and an Error enum -- bound head-on).
//
// Bound head-on off httplib's OWN classes:
//   httplib::Request   -- the request value type (method/path/body data members +
//                         has_header/get_header_value/get_param_value accessors)
//   httplib::Response  -- the response value type (status/reason/body + header accessors
//                         + set_content/set_header)
//   httplib::Result    -- what Client::Get/Post return: error()->Error, value()->Response&,
//                         operator bool, request-header accessors
//   httplib::Client    -- the REAL client, driven from Python (Get/Post/Put/Delete with
//                         string path/body/content-type + an explicit progress callback;
//                         the Headers/Params/multipart overloads are excluded so they skip)
//   httplib::Error     -- the client-side error enum (Connection on a closed-port connect)
//   httplib::StatusCode-- HTTP status-code enum (200/201/204/404/418 ...)
//
// The httptest::EchoServer fixture owns the SERVER side: it runs a real httplib::Server on a
// background thread (thread lifecycle + std::function handler registration are not
// Python-expressible) and echoes request data back. Everything the test asserts is produced
// by driving the bound Client from Python against that real server.
//
// std::function is bound via <nanobind/stl/function.h>: EVERY Client::Get/Delete overload
// takes a DownloadProgress (std::function) param, so without the function caster those verbs
// would drop entirely (the default nullptr is a C++26-unbindable default VALUE). With the
// caster the no-callback overloads still need the progress arg passed explicitly (a real
// lambda; None is not an empty std::function) -- the tests pass `lambda c,t: True` and the
// native oracle passes the identical always-continue callback, so both sides drive the same
// call. nb::exclude_ makes the binder skip every member whose signature mentions a type it
// still cannot represent (BINDER-0014): the Headers (unordered_multimap) / Params (multimap)
// associative containers (no multimap caster -- headers go through the string get_header_value
// / set_header accessors), and the streaming / multipart / internal-impl surface.
#include "httptest.h"

#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/function.h>

namespace nb = nanobind;

NB_MODULE(httplib_ext, m) {
    nb::reflect_<
        ^^httplib::Request,
        ^^httplib::Response,
        ^^httplib::Result,
        ^^httplib::Client,
        ^^httplib::Error,
        ^^httplib::StatusCode,
        ^^httptest,
        ^^nb::exclude_<
            ^^std::multimap,
            ^^std::unordered_multimap,
            ^^std::chrono::time_point,
            ^^httplib::MultipartFormData,
            ^^httplib::UploadFormData,
            ^^httplib::FormDataProvider,
            ^^httplib::DataSink,
            ^^httplib::ContentReader,
            ^^httplib::Stream,
            ^^httplib::UserData,
            ^^httplib::ClientImpl,
            ^^httplib::Range>
    >(m);
}
