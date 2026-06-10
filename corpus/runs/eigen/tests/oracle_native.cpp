// Native C++ ground-truth oracle for the eigen binding (Layer-1 differential).
// Drives the EXACT sequence test_bindings.py drives through the bound module --
// same vectors, same matrix, same operations, partly through the shared
// eigentest fixture -- and emits every observable as JSON. Shared compiler +
// shared (header-only) Eigen => any divergence is the binding layer's.
#include "../binding/eigentest.h"

#include <Eigen/Dense>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const std::string& k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') { e += "\\n"; }
            else { e += c; }
        }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const std::string& k, std::int64_t v) {
        kv.emplace_back(k, std::to_string(v));
    };
    // Doubles are emitted via a max-precision ostringstream: Python repr's
    // round-trip guarantee on the test side compares against these exactly.
    auto fmt_d = [](double v) {
        std::ostringstream o;
        o.precision(17);
        o << v;
        return o.str();
    };
    auto add_d = [&](const std::string& k, double v) { kv.emplace_back(k, fmt_d(v)); };

    using eigentest::Vec3;
    using eigentest::Mat3;

    // --- the shared scenario (mirrored verbatim in test_bindings.py) ---
    Vec3 v(1.0, 2.0, 3.0);
    add_d("v_x", v.x());
    add_d("v_y", v.y());
    add_d("v_z", v.z());
    add_d("v_idx0", v[0]);
    add_d("v_idx2", v[2]);
    add_d("v_call1", v(1));
    add_d("v_norm", v.norm());
    add_d("v_squared_norm", v.squaredNorm());
    add_d("v_sum", v.sum());
    add_d("v_prod", v.prod());
    add_d("v_mean", v.mean());
    add_d("v_min", v.minCoeff());
    add_d("v_max", v.maxCoeff());
    add_i("v_size", v.size());
    add_i("v_rows", v.rows());
    add_i("v_cols", v.cols());

    // fixture arithmetic (the eager-value face of Eigen's templated operators)
    Vec3 w = eigentest::add(v, eigentest::scale(v, 2.0));
    for (int i = 0; i < 3; ++i) add_d("w_" + std::to_string(i), w(i));
    add_d("dot_vv", eigentest::dot(v, v));
    Vec3 cx = eigentest::cross(Vec3(1, 0, 0), Vec3(0, 1, 0));
    for (int i = 0; i < 3; ++i) add_d("cross_" + std::to_string(i), cx(i));
    Vec3 nv = eigentest::sub(v, eigentest::negate(v));
    for (int i = 0; i < 3; ++i) add_d("subneg_" + std::to_string(i), nv(i));

    // a non-trivial (asymmetric, invertible) matrix built through the fixture
    Mat3 a = eigentest::mat3_from_rows(Vec3(2, 1, 0), Vec3(0, 3, 1), Vec3(1, 0, 4));
    add_d("a_det", a.determinant());
    add_d("a_trace", a.trace());
    add_d("a_norm", a.norm());
    add_d("a_sum", a.sum());
    add_d("a_01", a(0, 1));
    add_d("a_20", a(2, 0));

    Mat3 at = eigentest::transpose3(a);
    add_d("at_10", at(1, 0));
    add_d("at_02", at(0, 2));

    Mat3 ai = eigentest::inverse3(a);
    for (int i = 0; i < 3; ++i)
        add_d("ai_d" + std::to_string(i), ai(i, i));
    Mat3 prod = eigentest::matmul(a, ai);   // ~identity
    add_d("aai_trace", prod.trace());

    Vec3 mv = eigentest::matvec(a, v);
    for (int i = 0; i < 3; ++i) add_d("mv_" + std::to_string(i), mv(i));

    Vec3 sol = eigentest::solve3(a, v);
    for (int i = 0; i < 3; ++i) add_d("sol_" + std::to_string(i), sol(i));

    // symmetric eigenvalues (ascending, real) through the fixture
    Mat3 sym = eigentest::mat3_from_rows(Vec3(2, 0, 0), Vec3(0, 3, 0), Vec3(0, 0, 4));
    Vec3 ev = eigentest::sym_eigenvalues(sym);
    for (int i = 0; i < 3; ++i) add_d("ev_" + std::to_string(i), ev(i));

    // mutation through the bound surface
    Vec3 mu(v);                      // copy (BINDER-0013 path on the Python side)
    mu.setZero();
    add_d("after_setzero_sum", mu.sum());
    mu.setOnes();
    add_d("after_setones_sum", mu.sum());
    mu.setConstant(5.0);
    add_d("after_setconst_sum", mu.sum());
    Vec3 sc(v);
    sc *= 2.0;
    add_d("after_imul_1", sc(1));
    sc /= 2.0;
    add_d("after_idiv_1", sc(1));
    Vec3 nz(3.0, 0.0, 0.0);
    nz.normalize();
    add_d("after_normalize_0", nz(0));
    add_d("v_normalized_norm", v.normalized().norm());
    Mat3 idm(a);
    idm.setIdentity();
    add_d("ident_trace", idm.trace());
    add_d("ident_det", idm.determinant());
    Mat3 mset(a);
    eigentest::set_coeff(mset, 1, 2, 42.0);
    add_d("set_coeff_12", mset(1, 2));
    Vec3 vset(v);
    eigentest::vset_coeff(vset, 0, 7.5);
    add_d("vset_0", vset(0));

    // __str__ (the templated stream operator surfaced via ostringstream)
    {
        std::ostringstream o;
        o << v;
        add_s("str_v", o.str());
    }
    {
        std::ostringstream o;
        o << a;
        add_s("str_a", o.str());
    }

    // --- emit ---
    std::cout << "{";
    for (std::size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}\n";
    return 0;
}
