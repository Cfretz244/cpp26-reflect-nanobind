#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
struct Cell {
    int v = 0;
    int getInt() const noexcept { return v; }
    const void* getBlob() const noexcept { return &v; }
};
NB_MODULE(voidrepro_ext, m) {
    nb::reflect_<^^Cell>(m);
}
