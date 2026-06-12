// Emit-lane generator: renders the binding TU as plain nanobind source.
#include <mirrorbind/emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return mirrorbind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "abseil_hash_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
