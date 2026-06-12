// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^absl::InlinedVector<int, 4>, ^^absl::InlinedVector<double, 2>
