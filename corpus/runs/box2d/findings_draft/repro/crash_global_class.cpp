#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
struct V { float x, y; float len() const { return x*x+y*y; } };
NB_MODULE(d_ext, m){ nb::reflect_<^^V>(m); }
