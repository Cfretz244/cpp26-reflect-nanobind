// Single-stage binding module for the POD fixture. The entire body is one reflect_ call.
#include <nanobind/nb_reflect.h>
#include "pod_fixture.h"

namespace nb = nanobind;

NB_MODULE(pod_fixture_ext, m) {
    nb::reflect_<^^podfix>(m);
}
