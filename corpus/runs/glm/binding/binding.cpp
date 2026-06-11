// Bindings for a tractable subset of glm (1.0.1): the concrete vector specializations
// behind glm::vec3 / glm::vec4. glm is entirely templates; data members (x/y/z/w) and any
// non-template constructors/operators are the auto-bindable surface (templated members are
// gracefully skipped). The free-function API (length/dot/cross/normalize) is function
// templates, exercised via test-space wrappers rather than the binder.
#include <nanobind/nb_reflect.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(glm_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
