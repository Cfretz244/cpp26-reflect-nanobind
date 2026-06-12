// Generator: walks nlohmann::json + jsontest and emits trampoline source + the required
// <nanobind/stl/*.h> caster includes (string/map/vector/pair) for json's signatures. json has no
// virtuals, so the two-stage path is used purely for the automatic STL-caster includes.
#include <mirrorbind/codegen.h>
#include "jsontest.h"

namespace nb = nanobind;
namespace mb = mirrorbind;

int main(int argc, char** argv) {
    const char* out = (argc > 1) ? argv[1] : "trampolines.gen.h";
    return mb::write_trampolines(
               out, mb::emit_trampolines<^^nlohmann::json, ^^jsontest>())  // value_t/error_handler_t enums need no trampolines
               ? 0
               : 1;
}
