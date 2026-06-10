# LIB-0002 — Eigen 5.0.1 declares DenseBase::trace() but never defines it

- **Status:** worked around run-side (the declaration is excluded by member
  reflection in `corpus/runs/eigen/binding/binding.cpp`); candidate for an
  upstream Eigen report
- **Track:** library (Eigen 5.0.1 / gitlab.com/libeigen/eigen @ bc3b3987)
- **Found via:** `corpus/runs/eigen` Gate 4 link: `Undefined symbols:
  Eigen::DenseBase<Eigen::Matrix<double,3,1>>::trace() const`.

## Detail

`Eigen/src/Core/DenseBase.h:423` declares

```cpp
EIGEN_DEVICE_FUNC Scalar trace() const;
```

on `DenseBase<Derived>`, but no definition exists anywhere in the tree — the
real `trace()` is `MatrixBase<Derived>::trace()` (declared MatrixBase.h:310,
defined Redux.h:529). Ordinary C++ never notices: nothing odr-uses the
DenseBase declaration (any `m.trace()` call resolves to the MatrixBase
overload, which shadows it). A reflection-driven binder enumerates BOTH
declarations and binds each as a Python overload; the DenseBase one odr-uses
the missing definition and the module fails to link.

Dead declarations are invisible to reflection (there is no "has a definition"
query for functions in P2996/clang-p2996, unlike `has_complete_definition`
for types), so the run excludes the DenseBase `trace` members explicitly by
reflection; `MatrixBase::trace()` stays bound and is differentially tested.

Empirically there is no sibling on the bound surface: with only `trace`
excluded, the module links clean (every other bound facade member found its
definition).
