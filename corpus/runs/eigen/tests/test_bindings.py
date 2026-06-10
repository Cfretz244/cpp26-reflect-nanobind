"""Differential test for the eigen binding (Layer-1 + Layer-3).

Eigen::Matrix<double,3,1>/<double,3,3> are bound head-on with their CRTP facade
chains as REAL Python bases; the eigentest fixture supplies eager-value
arithmetic (Eigen's own operators are templates returning excluded expression
types). oracle_native.cpp drives the EXACT same scenario through native Eigen
and emits every observable; the assertions compare the bound module against
that ground truth. Layer 3 checks the Tier-5 themes structurally: the 8-deep
real Python base chain, the per-spec exclusion surface (matrix has no
__getitem__/x()/y()/z(); neither spec has transpose()/begin()/eigenvalues()),
and Python-side copy construction.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("eigen_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

Vec3 = m.MatrixDouble31031          # spec_camel_name of Matrix<double,3,1,0,3,1>
Mat3 = m.MatrixDouble33033


def vec():
    return Vec3(1.0, 2.0, 3.0)      # the real (x, y, z) constructor


def mat_a():
    return m.mat3_from_rows(Vec3(2, 1, 0), Vec3(0, 3, 1), Vec3(1, 0, 4))


def test_vector_ctor_and_access_differential():
    v = vec()
    assert v.x() == E["v_x"] and v.y() == E["v_y"] and v.z() == E["v_z"]
    assert v[0] == E["v_idx0"] and v[2] == E["v_idx2"]   # __getitem__ (vector-only)
    assert v(1) == E["v_call1"]                          # __call__ coeff access


def test_vector_reductions_differential():
    v = vec()
    assert v.norm() == E["v_norm"]
    assert v.squaredNorm() == E["v_squared_norm"]
    assert v.sum() == E["v_sum"]
    assert v.prod() == E["v_prod"]
    assert v.mean() == E["v_mean"]
    assert v.minCoeff() == E["v_min"]
    assert v.maxCoeff() == E["v_max"]
    assert v.size() == E["v_size"]
    assert v.rows() == E["v_rows"]
    assert v.cols() == E["v_cols"]


def test_fixture_arithmetic_differential():
    v = vec()
    w = m.add(v, m.scale(v, 2.0))
    assert [w(i) for i in range(3)] == [E[f"w_{i}"] for i in range(3)]
    assert m.dot(v, v) == E["dot_vv"]
    cx = m.cross(Vec3(1, 0, 0), Vec3(0, 1, 0))
    assert [cx(i) for i in range(3)] == [E[f"cross_{i}"] for i in range(3)]
    nv = m.sub(v, m.negate(v))
    assert [nv(i) for i in range(3)] == [E[f"subneg_{i}"] for i in range(3)]


def test_matrix_surface_differential():
    a = mat_a()
    assert a.determinant() == E["a_det"]
    assert a.trace() == E["a_trace"]
    assert a.norm() == E["a_norm"]
    assert a.sum() == E["a_sum"]
    assert a(0, 1) == E["a_01"] and a(2, 0) == E["a_20"]
    at = m.transpose3(a)
    assert at(1, 0) == E["at_10"] and at(0, 2) == E["at_02"]


def test_linear_algebra_differential():
    a, v = mat_a(), vec()
    ai = m.inverse3(a)
    assert [ai(i, i) for i in range(3)] == [E[f"ai_d{i}"] for i in range(3)]
    assert m.matmul(a, ai).trace() == E["aai_trace"]
    mv = m.matvec(a, v)
    assert [mv(i) for i in range(3)] == [E[f"mv_{i}"] for i in range(3)]
    sol = m.solve3(a, v)
    assert [sol(i) for i in range(3)] == [E[f"sol_{i}"] for i in range(3)]
    sym = m.mat3_from_rows(Vec3(2, 0, 0), Vec3(0, 3, 0), Vec3(0, 0, 4))
    ev = m.sym_eigenvalues(sym)
    assert [ev(i) for i in range(3)] == [E[f"ev_{i}"] for i in range(3)]


def test_mutation_differential():
    v = vec()
    mu = Vec3(v)                    # Python-side copy construction (BINDER-0013)
    mu.setZero()
    assert mu.sum() == E["after_setzero_sum"]
    assert v.sum() == E["v_sum"]    # the copy is distinct
    mu.setOnes()
    assert mu.sum() == E["after_setones_sum"]
    mu.setConstant(5.0)
    assert mu.sum() == E["after_setconst_sum"]
    sc = Vec3(v)
    sc *= 2.0                       # operator*=(Scalar) -> __imul__
    assert sc(1) == E["after_imul_1"]
    sc /= 2.0
    assert sc(1) == E["after_idiv_1"]
    nz = Vec3(3.0, 0.0, 0.0)
    nz.normalize()
    assert nz(0) == E["after_normalize_0"]
    assert v.normalized().norm() == E["v_normalized_norm"]
    idm = Mat3(mat_a())
    idm.setIdentity()
    assert idm.trace() == E["ident_trace"]
    assert idm.determinant() == E["ident_det"]
    mset = Mat3(mat_a())
    m.set_coeff(mset, 1, 2, 42.0)
    assert mset(1, 2) == E["set_coeff_12"]
    vset = Vec3(v)
    m.vset_coeff(vset, 0, 7.5)
    assert vset(0) == E["vset_0"]


def test_str_differential():
    # __str__ rides the templated operator<< via bind_stream_str's
    # requires-expression -- byte-for-byte against the native formatting.
    assert str(vec()) == E["str_v"]
    assert str(mat_a()) == E["str_a"]


# --- Layer 3: invariants (the Tier-5 themes, structurally) ---

def test_real_python_base_chain():
    # The whole CRTP facade chain is bound and wired as REAL Python bases:
    # Matrix -> PlainObjectBase -> MatrixBase -> DenseBase ->
    # DenseCoeffsBase<3/1/0> -> EigenBase. (The reachability rule made them
    # all signature-reachable; nothing is flattened.)
    v = vec()
    assert isinstance(v, m.PlainObjectBaseMatrixDouble31031)
    assert isinstance(v, m.MatrixBaseMatrixDouble31031)
    assert isinstance(v, m.DenseBaseMatrixDouble31031)
    assert isinstance(v, m.EigenBaseMatrixDouble31031)
    mro = [c.__name__ for c in Vec3.__mro__]
    assert mro == [
        "MatrixDouble31031",
        "PlainObjectBaseMatrixDouble31031",
        "MatrixBaseMatrixDouble31031",
        "DenseBaseMatrixDouble31031",
        "DenseCoeffsBaseMatrixDouble310313",
        "DenseCoeffsBaseMatrixDouble310311",
        "DenseCoeffsBaseMatrixDouble310310",
        "EigenBaseMatrixDouble31031",
        "object",
    ]
    # The two specs' facade chains are disjoint types.
    assert not isinstance(vec(), m.MatrixBaseMatrixDouble33033)


def test_facades_not_instantiable():
    # The facades have no public constructors (CRTP bases); binding them grew
    # no __init__ (the BINDER-0011 abstract-class contract).
    with pytest.raises(TypeError):
        m.MatrixBaseMatrixDouble31031()
    with pytest.raises(TypeError):
        m.DenseBaseMatrixDouble33033()


def test_per_spec_exclusions():
    # operator[] and x()/y()/z() are vector-only (their BODIES static_assert
    # on matrices): excluded on Mat3 by member reflection, kept on Vec3.
    a = mat_a()
    with pytest.raises(TypeError):
        a[0]
    assert not hasattr(a, "x")
    assert vec().x() == 1.0
    # Expression-template returns are excluded everywhere: the lazy API is
    # simply absent (the fixture provides the eager equivalents).
    for absent in ("transpose", "inverse", "diagonal", "head", "asDiagonal",
                   "begin", "end", "eigenvalues", "eulerAngles", "resize"):
        assert not hasattr(vec(), absent), absent
    # No expression/solver class leaked into the module.
    for absent in ("TransposeMatrixDouble31031", "CommaInitializerMatrixDouble31031"):
        assert not hasattr(m, absent), absent


def test_wrong_sized_ctors_excluded():
    # Matrix(x, y, z) is a plain member of EVERY Matrix whose body
    # static_asserts the size: excluded on Mat3 by member reflection (the
    # exclude_ per-member escape hatch), kept on Vec3.
    with pytest.raises(TypeError):
        Mat3(1.0, 2.0, 3.0)
    with pytest.raises(TypeError):
        Vec3(1.0, 2.0, 3.0, 4.0)
    assert Vec3(1.0, 2.0, 3.0).sum() == 6.0
