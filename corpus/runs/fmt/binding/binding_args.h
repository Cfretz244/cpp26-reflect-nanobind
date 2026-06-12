// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The to_string<T> instantiations bind under template-spec CamelCase names
// with an enable_if-NTTP suffix (BINDER-0003, open cosmetic).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^fmt::color, ^^fmt::terminal_color, ^^fmt::emphasis,                      \
    ^^fmt::format_int,                                                         \
    ^^fmt::to_string<int>, ^^fmt::to_string<double>,                           \
    ^^fmt::to_string<long long>,                                               \
    ^^fmttest
