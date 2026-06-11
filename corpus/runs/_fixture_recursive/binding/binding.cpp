#include <nanobind/nb_reflect.h>
#include <nanobind/stl/vector.h>   // single-stage: Node has a std::vector<Node> member
#include "binding_args.h"
namespace nb = nanobind;
NB_MODULE(tree_ext, m) { nb::reflect_<CORPUS_REFLECT_ARGS>(m); }
