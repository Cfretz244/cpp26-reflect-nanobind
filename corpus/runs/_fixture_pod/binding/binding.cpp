// Single-stage binding module for the POD fixture (constexpr lane).
#include <mirrorbind/reflect.h>
#include "binding_args.h"

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(pod_fixture_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
