// Emit-lane generator. The generated TU is the module's single TU, so its
// preamble carries TINYOBJLOADER_IMPLEMENTATION.
#include <nanobind/nb_reflect_emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return nanobind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "tinyobjloader_ext",
               "#define TINYOBJLOADER_IMPLEMENTATION\n"
               "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
