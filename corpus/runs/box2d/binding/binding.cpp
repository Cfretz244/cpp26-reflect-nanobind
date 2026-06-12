// Single-stage bindings for box2d v2.4.2 (Tier 4: a real rigid-body physics
// engine bound head-on -- math value types, the world/body/fixture object
// graph, plain option structs, and concrete shape classes).
//
// The bind set lives ONCE in binding_args.h (shared with the emit generator
// gen_emit.cpp); the library headers and config live in binding_includes.h.
// box2d's public types are all GLOBAL-namespace classes -- every concrete type
// is listed explicitly in the pack; the internal facade types are excluded
// (see binding_args.h). box2d uses no STL type-casters on the bound surface,
// so no <nanobind/stl/*.h> includes are needed.
#include <mirrorbind/reflect.h>
#include "binding_args.h"  // bind set defined once (shared with the emit generator)

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(box2d_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
