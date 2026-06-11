// Emit-lane generator: renders the binding TU as plain nanobind source.
#include <nanobind/nb_reflect_emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return nanobind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "fast_float_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
