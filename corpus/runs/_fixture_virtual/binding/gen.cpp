// Generator: walks the shapes namespace and writes trampoline source (+ any needed STL
// caster includes) for every class with overridable virtuals to argv[1]. Run at build time.
#include <nanobind/nb_reflect_codegen.h>
#include "shapes.h"

namespace nb = nanobind;

int main(int argc, char** argv) {
    const char* out = (argc > 1) ? argv[1] : "trampolines.gen.h";
    return nb::write_trampolines(out, nb::emit_trampolines<^^shapes>()) ? 0 : 1;
}
