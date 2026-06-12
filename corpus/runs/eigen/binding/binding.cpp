// Single-stage bindings for Eigen 5.0.1 (Tier 5: the expression-template
// stress test -- the run mb::exclude_ was built for).
//
// Vector3d (Matrix<double,3,1>) and Matrix3d (Matrix<double,3,3>) bind
// head-on; their CRTP facade chains (PlainObjectBase -> MatrixBase ->
// DenseBase -> DenseCoeffsBase<3/1/0> -> EigenBase) are signature-reachable,
// enter the bind set, and wire up as REAL Python bases. The exclusion marker
// makes that closure finite and well-formed:
//   * expression/view templates (Transpose, CwiseUnaryOp, Block, ...): every
//     facade mints deeper specs of these (Transpose<Transpose<...>>), so
//     discovery would diverge; some minted specs are outright hard errors to
//     walk (NestByValue<expr>'s enable_if members, SparseView without
//     <Eigen/SparseCore>).
//   * solver objects and Eigen::internal: out of scope for a value binding.
//   * per-MEMBER exclusions (by reflection -- the escape hatch for unowned
//     code): members whose BODIES are lazily ill-formed for a bound spec.
//     Matrix's fixed-size convenience ctors ((x,y), (x,y,z), (x,y,z,w)) are
//     plain members of EVERY Matrix that static_assert the size in the body;
//     a few facade members do the same (eulerAngles asserts 3x3, unit-vector
//     statics assert vector shape, eigenvalues/operatorNorm assert square).
//     No reflection query can see a body, so they are listed by their exact
//     member reflections, computed below from members_of.
#include <mirrorbind/reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/complex.h>
#include "binding_args.h"  // bind set + exclusion marker defined once

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(eigen_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
