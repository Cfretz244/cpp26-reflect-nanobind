// Constexpr-lane trampoline generator (two-stage): writes trampoline source
// (+ STL caster includes) for every class with overridable virtuals.
#include <mirrorbind/codegen.h>
#include "binding_includes.h"

namespace nb = nanobind;
namespace mb = mirrorbind;

int main(int argc, char** argv) {
    const char* out = (argc > 1) ? argv[1] : "trampolines.gen.h";
    return mb::write_trampolines(out, mb::emit_trampolines<^^shapes>()) ? 0 : 1;
}
