// Emit-lane generator: renders the COMPLETE binding TU (incl. its own STL
// caster includes) as plain nanobind source; json has no virtuals, so no
// trampoline marker is needed.
#include <nanobind/nb_reflect_emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return nanobind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "json_ext", "#include \"jsontest.h\"\n")
               ? 0
               : 1;
}
