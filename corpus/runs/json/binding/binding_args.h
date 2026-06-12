// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// jsontest.h doubles as the both-compilers include set (its annotation macro
// is feature-guarded), so there is no separate binding_includes.h here.
#pragma once
#include <mirrorbind/reflect.h>
#include "jsontest.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^nlohmann::json, ^^nlohmann::detail::value_t,                             \
    ^^nlohmann::detail::error_handler_t, ^^jsontest
