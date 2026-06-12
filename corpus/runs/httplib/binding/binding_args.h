// The reflect_ pack for the httplib run, defined ONCE for every backend
// consumer (P2996-only). Mirrors reflect_args in meta.toml.
//   httplib::Request/Response  -- the value types
//   httplib::Result            -- Client::Get/Post return type
//   httplib::Client            -- the REAL loopback client (driven from Python)
//   httplib::Error/StatusCode  -- the enums (differentially checked)
//   httptest                   -- the EchoServer loopback fixture namespace
// mb::exclude_ makes every member whose signature mentions an un-representable
// type opaque (the multimap-based Headers/Params, the multipart/streaming/
// internal-impl surface); std::function is NOT excluded -- it is bound via
// <nanobind/stl/function.h> so the progress-callback Get/Delete overloads bind.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
// The pack splices ^^mb::exclude_, so the alias must be in scope wherever the
// macro expands (binding.cpp already declares it, but gen_emit.cpp does not).
namespace nb = nanobind;
namespace mb = mirrorbind;
#define CORPUS_REFLECT_ARGS                                                    \
    ^^httplib::Request,                                                        \
    ^^httplib::Response,                                                       \
    ^^httplib::Result,                                                         \
    ^^httplib::Client,                                                         \
    ^^httplib::Error,                                                          \
    ^^httplib::StatusCode,                                                     \
    ^^httptest,                                                                \
    ^^mb::exclude_<                                                            \
        ^^std::multimap,                                                       \
        ^^std::unordered_multimap,                                             \
        ^^std::chrono::time_point,                                             \
        ^^httplib::MultipartFormData,                                          \
        ^^httplib::UploadFormData,                                             \
        ^^httplib::FormDataProvider,                                           \
        ^^httplib::DataSink,                                                   \
        ^^httplib::ContentReader,                                              \
        ^^httplib::Stream,                                                     \
        ^^httplib::UserData,                                                   \
        ^^httplib::ClientImpl,                                                 \
        ^^httplib::Range>
