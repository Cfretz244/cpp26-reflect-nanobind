// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^tinyobj::index_t, ^^tinyobj::mesh_t, ^^tinyobj::shape_t,                 \
    ^^tinyobj::attrib_t, ^^tinyobj::material_t, ^^objtest
