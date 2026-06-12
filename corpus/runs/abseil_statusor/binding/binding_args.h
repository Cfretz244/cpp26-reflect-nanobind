// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// Three real StatusOr<T> specializations bound head-on (ok/status/value), with
// value() re-exported through StatusOr's using-declaration from the PRIVATE
// internal_statusor::OperatorBase<T> base via entity-proxy support
// (-fentity-proxy-reflection; TC-0003). Status + StatusCode are bound in-module
// (status() returns const Status&), and the sotest namespace supplies the
// ok_*/err_* factories + get_* differential accessors.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^absl::StatusOr<int>, ^^absl::StatusOr<std::string>,                      \
    ^^absl::StatusOr<double>, ^^absl::Status, ^^absl::StatusCode, ^^sotest
