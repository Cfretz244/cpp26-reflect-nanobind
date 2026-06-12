// The reflect_ pack for the eigen run, defined ONCE for every backend
// consumer (P2996-only). Vector3d/Matrix3d bind head-on; the exclusion
// marker (built programmatically below) is what makes their CRTP facade
// closure finite and well-formed -- expression/view templates, solvers,
// Eigen::internal, and the per-MEMBER reflections of lazily-ill-formed
// bodies (fixed-size convenience ctors, shape-asserting facade members).
// See binding.cpp's header comment for the full rationale.
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"

using Vec3 = Eigen::Matrix<double, 3, 1>;
using Mat3 = Eigen::Matrix<double, 3, 3>;
using CVec1 = Eigen::Matrix<std::complex<double>, 1, 1>;   // from eigenvalues()
using CVec3 = Eigen::Matrix<std::complex<double>, 3, 1>;   // declarations

namespace eigen_args_detail {


namespace sm = std::meta;

// All non-template constructors of M whose parameters are all `const Scalar&`
// but whose count cannot initialize an M (the body's
// EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE would fire at nb::init
// instantiation), plus the raw-pointer ctors (Matrix(const Scalar*) -- an
// unbindable footgun from Python).
consteval void collect_bad_matrix_ctors(sm::info M, std::size_t size,
                                        std::vector<sm::info>& out) {
    // M may be reached through a `using` alias (Vec3/Mat3/...); GCC 16's
    // template_arguments_of THROWS on a typedef reflection (the clang-p2996
    // fork auto-dealiased). Dealias first -- members_of/parameters_of below
    // already tolerate the alias, this is the one strict metafunction.
    sm::info scalar = sm::template_arguments_of(sm::dealias(M))[0];
    for (auto m : sm::members_of(M, sm::access_context::unchecked())) {
        if (!sm::is_constructor(m) || sm::is_template(m) || sm::is_deleted(m))
            continue;
        auto ps = sm::parameters_of(m);
        if (ps.empty())
            continue;
        bool all_scalar = true, any_ptr = false;
        for (auto p : ps) {
            auto t = sm::remove_cvref(sm::type_of(p));
            if (sm::is_pointer_type(t))
                any_ptr = true;
            else if (t != scalar)
                all_scalar = false;
        }
        if (any_ptr)
            out.push_back(m);
        else if (all_scalar && ps.size() >= 1 && ps.size() <= 4 &&
                 ps.size() != size)
            out.push_back(m);
    }
}

// Every member (or member template) of M named `name`, across its whole
// public-base subtree -- for facade members whose bodies are lazily
// ill-formed on some bound specialization. `arity` restricts to a specific
// parameter count (npos = all): setConstant(Index size, const Scalar&) is the
// vector-resize form whose body calls resize(size), while setConstant(const
// Scalar&) is fine everywhere.
consteval void collect_members_named(sm::info M, std::string_view name,
                                     std::vector<sm::info>& out,
                                     std::size_t arity = std::size_t(-1)) {
    std::vector<sm::info> owners{M};
    nanobind::detail::collect_public_base_subtree(M, owners);
    for (auto o : owners)
        for (auto m : sm::members_of(o, sm::access_context::unchecked()))
            if (sm::has_identifier(m) &&
                sm::identifier_of(m) == name &&
                (arity == std::size_t(-1) ||
                 (sm::is_function(m) && !sm::is_template(m) &&
                  sm::parameters_of(m).size() == arity)))
                out.push_back(m);
}

consteval sm::info eigen_excluded_marker() {
    std::vector<sm::info> args;
    auto add = [&](sm::info e) { args.push_back(sm::reflect_constant(e)); };

    // -- namespaces --
    add(^^Eigen::internal);
    // -- expression / view / plumbing templates (the divergence engines) --
    for (sm::info t : {^^Eigen::Transpose, ^^Eigen::Diagonal, ^^Eigen::Block,
                       ^^Eigen::VectorBlock, ^^Eigen::Reverse,
                       ^^Eigen::Replicate, ^^Eigen::Reshaped,
                       ^^Eigen::NestByValue, ^^Eigen::ArrayWrapper,
                       ^^Eigen::MatrixWrapper, ^^Eigen::CwiseUnaryOp,
                       ^^Eigen::CwiseBinaryOp, ^^Eigen::CwiseNullaryOp,
                       ^^Eigen::CwiseTernaryOp, ^^Eigen::CwiseUnaryView,
                       ^^Eigen::Map, ^^Eigen::Ref, ^^Eigen::CommaInitializer,
                       ^^Eigen::WithFormat, ^^Eigen::Select,
                       ^^Eigen::VectorwiseOp, ^^Eigen::Homogeneous,
                       ^^Eigen::DiagonalWrapper, ^^Eigen::Solve,
                       ^^Eigen::Inverse, ^^Eigen::Product,
                       ^^Eigen::PartialReduxExpr, ^^Eigen::IndexedView,
                       ^^Eigen::TriangularView, ^^Eigen::SelfAdjointView,
                       ^^Eigen::SkewSymmetricWrapper, ^^Eigen::RealView,
                       ^^Eigen::ForceAlignedAccess, ^^Eigen::NoAlias,
                       ^^Eigen::PermutationWrapper, ^^Eigen::PermutationBase,
                       ^^Eigen::PermutationMatrix, ^^Eigen::SparseView})
        add(t);
    // -- solver / decomposition objects --
    for (sm::info t : {^^Eigen::LLT, ^^Eigen::LDLT, ^^Eigen::PartialPivLU,
                       ^^Eigen::FullPivLU, ^^Eigen::HouseholderQR,
                       ^^Eigen::ColPivHouseholderQR,
                       ^^Eigen::FullPivHouseholderQR,
                       ^^Eigen::CompleteOrthogonalDecomposition,
                       ^^Eigen::JacobiSVD, ^^Eigen::BDCSVD,
                       ^^Eigen::EigenSolver, ^^Eigen::GeneralizedEigenSolver,
                       ^^Eigen::SelfAdjointEigenSolver,
                       ^^Eigen::HessenbergDecomposition, ^^Eigen::RealSchur,
                       ^^Eigen::ComplexSchur, ^^Eigen::ComplexEigenSolver,
                       ^^Eigen::Tridiagonalization, ^^Eigen::RealQZ,
                       ^^Eigen::HouseholderSequence, ^^Eigen::JacobiRotation})
        add(t);

    // -- per-member: lazily-ill-formed bodies (see file comment) --
    std::vector<sm::info> bad;
    collect_bad_matrix_ctors(^^Vec3, 3, bad);
    collect_bad_matrix_ctors(^^Mat3, 9, bad);
    collect_bad_matrix_ctors(^^CVec1, 1, bad);
    collect_bad_matrix_ctors(^^CVec3, 3, bad);
    for (sm::info M : {^^Vec3, ^^Mat3, ^^CVec1, ^^CVec3}) {
        // eulerAngles/canonicalEulerAngles: bodies static_assert 3x3 (only
        // Mat3 qualifies, and there they return a bindable Vec3 -- but keep
        // the surface uniform and skip them everywhere; the fixture provides
        // eager equivalents). isSkewSymmetric's body mixes transpose shapes
        // (vector sizes static_assert).
        collect_members_named(M, "eulerAngles", bad);
        collect_members_named(M, "canonicalEulerAngles", bad);
        collect_members_named(M, "isSkewSymmetric", bad);
        // unit-vector statics: bodies static_assert vector shape.
        for (std::string_view n : {"Unit", "UnitX", "UnitY", "UnitZ", "UnitW"})
            collect_members_named(M, n, bad);
        // square-only spectral members: bodies static_assert squareness.
        for (std::string_view n : {"eigenvalues", "operatorNorm"})
            collect_members_named(M, n, bad);
        // intCast: casts to integer scalars, irrelevant surface.
        collect_members_named(M, "intCast", bad);
        // More lazily-ill-formed bodies, found at Gate 4 (each static_asserts
        // a shape the bound spec may not have): value() is 1x1-only,
        // setUnit/setLinSpaced/unitOrthogonal are vector-only, the resize
        // family is for dynamic/vector shapes (meaningless on fixed-size specs
        // anyway). w() (needs size >= 4) and resize() (matrix-ill-formed) have
        // CONSTEXPR bodies GCC 16 instantiates when their reflection is
        // materialized (GCC-6); they are excluded by NAME below
        // (nb::exclude_member_) so their reflection is never formed.
        for (std::string_view n :
             {"value", "setUnit", "setLinSpaced", "setEqualSpaced",
              "unitOrthogonal", "resizeLike", "conservativeResize",
              "conservativeResizeLike"})
            collect_members_named(M, n, bad);
        // The vector-resize setConstant(Index, const Scalar&): its body calls
        // resize(size). The 1-arg setConstant(const Scalar&) stays bound.
        collect_members_named(M, "setConstant", bad, 2);
        // Same shape: setZero/setOnes/setRandom(Index) resize a vector in the
        // body. The 2-arg (rows, cols / NoChange_t) variants are meaningless
        // on fixed-size specs AND -- bound on the derived PlainObjectBase
        // facade -- they SHADOW the useful 0-arg DenseBase forms in Python's
        // MRO (nanobind overload chains do not merge across base classes).
        // Exclude both arities; the 0-arg forms then resolve. setConstant's
        // 3-arg (rows, cols, value) variants shadow the 1-arg form the same
        // way.
        for (std::string_view n : {"setZero", "setOnes", "setRandom"}) {
            collect_members_named(M, n, bad, 1);
            collect_members_named(M, n, bad, 2);
        }
        collect_members_named(M, "setConstant", bad, 3);
        // STL iterators: on matrices the iterator typedef is VOID (begin()
        // declares as `void()` and its body static_asserts); on vectors it is
        // an internal:: type. No useful Python surface either way.
        for (std::string_view n : {"begin", "end", "cbegin", "cend",
                                   "rbegin", "rend", "crbegin", "crend"})
            collect_members_named(M, n, bad);
    }
    // operator[] is vector-only (its body static_asserts), and the x/y/z
    // accessors call it in THEIR bodies: exclude them on the matrix spec, keep
    // them (__getitem__, .x()/.y()/.z()) on the 3-vectors. operator[]'s
    // constexpr body is instantiated by GCC the moment its reflection is
    // materialized (GCC-6), so it -- like x/y/z -- is excluded by name below
    // (nb::exclude_member_<^^Mat3, "__getitem__">), never by reflection.
    // LIB-0002: Eigen 5.0.1 declares DenseBase::trace() but never DEFINES it
    // (the real trace lives on MatrixBase, Redux.h) -- a dead declaration
    // that normal use never odr-uses but a bound method does: undefined
    // symbol at link. Exclude it on the DenseBase facades only; the
    // MatrixBase one stays bound.
    for (sm::info D : {^^Eigen::DenseBase<Vec3>, ^^Eigen::DenseBase<Mat3>,
                       ^^Eigen::DenseBase<CVec1>, ^^Eigen::DenseBase<CVec3>})
        for (auto m : sm::members_of(D, sm::access_context::unchecked()))
            if (sm::has_identifier(m) &&
                sm::identifier_of(m) == "trace")
                bad.push_back(m);
    for (auto m : bad)
        add(m);

    // -- by-NAME member exclusions (nb::exclude_member_<Derived, "name">) --
    // For members whose CONSTEXPR body GCC 16 instantiates the moment their
    // reflection is materialized as an NTTP (GCC-6: reflect_constant /
    // define_static_array lift), and whose body is lazily ill-formed for the
    // bound spec. Listing their reflection in exclude_ (as `bad` above does)
    // would itself be the hard error, so they are named instead -- the owner
    // is the DERIVED spec (a class; safe to materialize), and the binder drops
    // any matching member before the lift. Honored on clang too (identical
    // surface). w(): vector-only, size >= 4 (errors on every bound spec).
    add(^^nanobind::exclude_member_<^^Vec3, "w">);
    add(^^nanobind::exclude_member_<^^Mat3, "w">);
    add(^^nanobind::exclude_member_<^^CVec1, "w">);
    add(^^nanobind::exclude_member_<^^CVec3, "w">);
    // On the 3x3, the vector-only coeff accessors x/y/z are matrix-ill-formed.
    add(^^nanobind::exclude_member_<^^Mat3, "x">);
    add(^^nanobind::exclude_member_<^^Mat3, "y">);
    add(^^nanobind::exclude_member_<^^Mat3, "z">);
    // operator[] -> __getitem__ (vector-only on a matrix).
    add(^^nanobind::exclude_member_<^^Mat3, "__getitem__">);
    // resize() is excluded on EVERY bound spec (the surface omits it -- the
    // fixture has no resize story). On the 3x3 its constexpr body is
    // matrix-ill-formed (the GCC-6 trigger); on the vectors it is well-formed
    // but still unwanted -- name-exclude it uniformly so one rule shape covers
    // all four specs.
    add(^^nanobind::exclude_member_<^^Vec3, "resize">);
    add(^^nanobind::exclude_member_<^^Mat3, "resize">);
    add(^^nanobind::exclude_member_<^^CVec1, "resize">);
    add(^^nanobind::exclude_member_<^^CVec3, "resize">);
    // On the 1x1 (complex) vector, y()/z() index past the end.
    add(^^nanobind::exclude_member_<^^CVec1, "y">);
    add(^^nanobind::exclude_member_<^^CVec1, "z">);

    return sm::substitute(^^nanobind::exclude_, args);
}


}  // namespace eigen_args_detail

#define CORPUS_REFLECT_ARGS                                                    \
    ^^Eigen::Matrix<double, 3, 1>, ^^Eigen::Matrix<double, 3, 3>,              \
    eigen_args_detail::eigen_excluded_marker(), ^^eigentest
