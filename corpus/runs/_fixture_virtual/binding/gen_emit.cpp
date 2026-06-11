// Emit-lane generator: the generated TU inlines the trampolines itself
// (trampoline_all_ in the shared pack), no second stage needed.
#include <nanobind/nb_reflect_emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return nanobind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "shapes_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
