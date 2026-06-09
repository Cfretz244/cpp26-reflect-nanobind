// Native C++ oracle for the linalg vec subset: exercises the same API the bindings expose
// (construct vec<float,3>/vec<float,2>, data members, operator[]) and prints ground-truth JSON.
#include "linalg.h"
#include <cstdio>

int main() {
    linalg::vec<float, 3> v(1.5f, 2.5f, 3.5f);
    linalg::vec<float, 2> w(4.0f, 5.0f);

    std::printf(
        "{\n"
        "  \"v_x\": %.9g, \"v_y\": %.9g, \"v_z\": %.9g,\n"
        "  \"v_idx0\": %.9g, \"v_idx1\": %.9g, \"v_idx2\": %.9g,\n"
        "  \"w_x\": %.9g, \"w_y\": %.9g, \"w_idx0\": %.9g, \"w_idx1\": %.9g\n"
        "}\n",
        (double)v.x, (double)v.y, (double)v.z,
        (double)v[0], (double)v[1], (double)v[2],
        (double)w.x, (double)w.y, (double)w[0], (double)w[1]);
    return 0;
}
