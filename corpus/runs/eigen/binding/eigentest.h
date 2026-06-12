// eigentest -- the fixture half of the eigen run's binding surface.
//
// Eigen's arithmetic operators are function TEMPLATES returning expression
// templates: intrinsically outside the binder's model (a member template binds
// only via an all-defaulted default instantiation, and expression returns are
// mb::exclude_'d). These free functions supply the eager-VALUE equivalents,
// taking and returning the bound Matrix specializations by value -- so every
// call round-trips Python -> bound Vec3/Mat3 -> Eigen kernel -> bound result,
// exercising the registered-type caster path that the differential suite
// compares against the native oracle.
#pragma once

#include <Eigen/Dense>


namespace eigentest {

using Vec3 = Eigen::Matrix<double, 3, 1>;
using Mat3 = Eigen::Matrix<double, 3, 3>;

inline Vec3 add(const Vec3& a, const Vec3& b) { return a + b; }
inline Vec3 sub(const Vec3& a, const Vec3& b) { return a - b; }
inline Vec3 scale(const Vec3& a, double s) { return a * s; }
inline Vec3 negate(const Vec3& a) { return -a; }
inline double dot(const Vec3& a, const Vec3& b) { return a.dot(b); }
inline Vec3 cross(const Vec3& a, const Vec3& b) { return a.cross(b); }

inline Mat3 madd(const Mat3& a, const Mat3& b) { return a + b; }
inline Mat3 matmul(const Mat3& a, const Mat3& b) { return a * b; }
inline Vec3 matvec(const Mat3& m, const Vec3& v) { return m * v; }
inline Mat3 transpose3(const Mat3& m) { return m.transpose(); }
inline Mat3 inverse3(const Mat3& m) { return m.inverse(); }
inline Vec3 solve3(const Mat3& A, const Vec3& b) {
    return A.partialPivLu().solve(b);
}
// Eigenvalues of a SYMMETRIC matrix are real: returned as a sorted Vec3
// (SelfAdjointEigenSolver's ascending order).
inline Vec3 sym_eigenvalues(const Mat3& m) {
    return Eigen::SelfAdjointEigenSolver<Mat3>(m).eigenvalues();
}

// Population helpers: Mat3 has no bindable value constructor (the 9-scalar
// form does not exist; the comma-initializer is a C++ syntactic device), and
// the bound operator() returns the coefficient BY VALUE to Python.
inline Mat3 mat3_from_rows(const Vec3& r0, const Vec3& r1, const Vec3& r2) {
    Mat3 m;
    m.row(0) = r0.transpose();
    m.row(1) = r1.transpose();
    m.row(2) = r2.transpose();
    return m;
}
inline void set_coeff(Mat3& m, Eigen::Index i, Eigen::Index j, double v) {
    m(i, j) = v;
}
inline void vset_coeff(Vec3& v, Eigen::Index i, double x) { v(i) = x; }

}  // namespace eigentest
