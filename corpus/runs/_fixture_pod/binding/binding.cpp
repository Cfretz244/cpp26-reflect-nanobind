// Single-stage binding module for the POD fixture (constexpr lane).
#include <nanobind/nb_reflect.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(pod_fixture_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
