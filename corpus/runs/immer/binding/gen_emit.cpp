// Emit-lane generator: renders the immer binding TU as plain nanobind source.
#include <mirrorbind/emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return mirrorbind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "immer_ext",
               "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
