#include <nanobind/nb_reflect.h>
namespace synth3 {
struct Conn {
    int getInfo() const { return 1; }              // instance method
    static int getInfo(int x) { return x; }        // static, SAME name
};
}
namespace nb = nanobind;
NB_MODULE(synth3_ext, m) { nb::reflect_<^^synth3::Conn>(m); }
