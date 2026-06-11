// Constexpr-lane trampoline generator (two-stage): writes trampoline source
// (+ STL caster includes) for every class with overridable virtuals.
#include <nanobind/nb_reflect_codegen.h>
#include "binding_includes.h"

namespace nb = nanobind;

int main(int argc, char** argv) {
    const char* out = (argc > 1) ? argv[1] : "trampolines.gen.h";
    return nb::write_trampolines(out, nb::emit_trampolines<^^shapes>()) ? 0 : 1;
}
