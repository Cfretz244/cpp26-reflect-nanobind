// Emit-lane generator: renders the binding TU as plain nanobind source.
// The preamble re-includes binding_includes.h (httptest.h -> httplib.h + the
// EchoServer fixture); the emit backend appends the STL caster includes the
// signatures need (string/vector/pair/map/unordered_map/function).
#include <mirrorbind/emit.h>
#include "binding_args.h"

int main(int argc, char** argv) {
    return mirrorbind::write_bindings<CORPUS_REFLECT_ARGS>(
               argv[1], "httplib_ext", "#include \"binding_includes.h\"\n")
               ? 0
               : 1;
}
