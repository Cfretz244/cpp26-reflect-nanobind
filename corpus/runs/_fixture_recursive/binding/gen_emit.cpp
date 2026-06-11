// Emit-lane generator (the generated TU emits its own caster includes).
#include <nanobind/nb_reflect_emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return nanobind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "tree_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
