#include <mirrorbind/reflect.h>
#include <nanobind/stl/vector.h>   // single-stage: Node has a std::vector<Node> member
#include "binding_args.h"
namespace nb = nanobind;
namespace mb = mirrorbind;
NB_MODULE(tree_ext, m) { mb::reflect_<CORPUS_REFLECT_ARGS>(m); }
