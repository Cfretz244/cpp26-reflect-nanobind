// Native C++ oracle for the linalg subset: exercises the same API the bindings expose
// (the instantiate_-minted vec<T,M> and mat<float,M,N> grids: data members, operator[],
// xy() truncation, mat columns/rows/scalar fill) and prints ground-truth JSON.
#include "linalg.h"
#include <cstdio>

int main() {
    linalg::vec<float, 3> v(1.5f, 2.5f, 3.5f);
    linalg::vec<float, 2> w(4.0f, 5.0f);

    // the product-grid specs beyond float
    linalg::vec<double, 3> vd(0.5, 1.25, -2.0);
    linalg::vec<int, 3> vi(7, 8, 9);
    linalg::vec<int, 2> vixy = vi.xy();
    linalg::vec<int, 4> vs(5);                       // explicit scalar-fill ctor

    // the mat grid: columns are vec<float,M>, row(i) is vec<float,N>
    linalg::mat<float, 2, 2> m22({1.0f, 2.0f}, {3.0f, 4.0f});
    linalg::vec<float, 2> m22r0 = m22.row(0);
    linalg::mat<float, 2, 3> m23({1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f});
    linalg::vec<float, 3> m23r1 = m23.row(1);
    linalg::mat<float, 3, 3> mf(2.0f);               // explicit scalar-fill ctor

    std::printf(
        "{\n"
        "  \"v_x\": %.9g, \"v_y\": %.9g, \"v_z\": %.9g,\n"
        "  \"v_idx0\": %.9g, \"v_idx1\": %.9g, \"v_idx2\": %.9g,\n"
        "  \"w_x\": %.9g, \"w_y\": %.9g, \"w_idx0\": %.9g, \"w_idx1\": %.9g,\n"
        "  \"vd_x\": %.17g, \"vd_y\": %.17g, \"vd_z\": %.17g, \"vd_idx2\": %.17g,\n"
        "  \"vi_x\": %d, \"vi_y\": %d, \"vi_z\": %d,\n"
        "  \"vixy_x\": %d, \"vixy_y\": %d,\n"
        "  \"vs_x\": %d, \"vs_w\": %d,\n"
        "  \"m22_c0x\": %.9g, \"m22_c0y\": %.9g, \"m22_c1x\": %.9g, \"m22_c1y\": %.9g,\n"
        "  \"m22_r0x\": %.9g, \"m22_r0y\": %.9g,\n"
        "  \"m23_c2x\": %.9g, \"m23_c2y\": %.9g,\n"
        "  \"m23_r1x\": %.9g, \"m23_r1y\": %.9g, \"m23_r1z\": %.9g,\n"
        "  \"mf_c1y\": %.9g, \"mf_c2z\": %.9g\n"
        "}\n",
        (double)v.x, (double)v.y, (double)v.z,
        (double)v[0], (double)v[1], (double)v[2],
        (double)w.x, (double)w.y, (double)w[0], (double)w[1],
        vd.x, vd.y, vd.z, vd[2],
        vi.x, vi.y, vi.z,
        vixy.x, vixy.y,
        vs.x, vs.w,
        (double)m22[0].x, (double)m22[0].y, (double)m22[1].x, (double)m22[1].y,
        (double)m22r0.x, (double)m22r0.y,
        (double)m23[2].x, (double)m23[2].y,
        (double)m23r1.x, (double)m23r1.y, (double)m23r1.z,
        (double)mf[1].y, (double)mf[2].z);
    return 0;
}
