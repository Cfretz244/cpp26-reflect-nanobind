// Emit-lane generator: renders the CLI11 binding TU as plain nanobind source
// (compiled by the PRODUCTION compiler in stage 2). The bind set + exclude
// marker come from binding_args.h (shared with the constexpr lane's
// binding.cpp); the preamble re-includes binding_includes.h so the generated
// source sees the library types.
#include <mirrorbind/emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return mirrorbind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "cli11_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
