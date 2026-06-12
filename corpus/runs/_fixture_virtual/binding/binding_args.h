// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// trampoline_all_ gives the emit lane the same rule the constexpr lane's
// two-stage codegen applies: every class with overridable virtuals gets a
// trampoline (the constexpr lane treats the marker as inert configuration).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS ^^shapes, ^^mirrorbind::trampoline_all_
