// Bindings for a tractable subset of sgorsten/linalg (v2.2).
// linalg is entirely templates; the auto-bindable surface is the concrete vector
// specialization vec<float,3> (data members x/y/z, constructors, operator[] -> __getitem__).
// The rich free-function API (dot/length/normalize/...) is *function templates*, which the
// binder does not auto-discover; a couple are exposed explicitly as wrappers in test space.
#include <nanobind/nb_reflect.h>
#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(linalg_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
