// Generator: walks nlohmann::json + jsontest and emits trampoline source + the required
// <nanobind/stl/*.h> caster includes (string/map/vector/pair) for json's signatures. json has no
// virtuals, so the two-stage path is used purely for the automatic STL-caster includes.
#include <nanobind/nb_reflect_codegen.h>
#include "jsontest.h"

namespace nb = nanobind;

int main(int argc, char** argv) {
    const char* out = (argc > 1) ? argv[1] : "trampolines.gen.h";
    return nb::write_trampolines(
               out, nb::emit_trampolines<^^nlohmann::json, ^^jsontest>())  // value_t/error_handler_t enums need no trampolines
               ? 0
               : 1;
}
