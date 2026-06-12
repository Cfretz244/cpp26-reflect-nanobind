// The reflect_ pack for the fast_float run, defined ONCE for every backend
// consumer (P2996-only). Mirrors the reflect_args in meta.toml.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^fast_float::chars_format, \
    ^^fast_float::parse_options_t<char>, \
    ^^fast_float::from_chars_result_t<char>, \
    ^^fixture
