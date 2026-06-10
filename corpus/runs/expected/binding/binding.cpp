// Single-stage bindings for TartanLlama/expected v1.3.1 (Tier 3: error-channel
// vocabulary type). Three real expected<T,E> specializations bound head-on --
// int/std::string value-or-error, the role-swapped std::string/int, and the
// no-value success channel void/std::string. tl::unexpected<std::string>/<int>
// are listed for their REAL non-template ctors (constructible from a Python
// str/int directly); tl::bad_expected_access<std::string> binds as a class
// (ctor + error()/what()); the extest fixtures supply factories (the
// converting ctors are templates) and thin wrappers over the all-template
// operator== / value_or / monadic fronts.
//
// This surface formerly sat at outcome B behind a four-bug cascade, all fixed:
//   - TC-0008 (codegen ICE): tl's namespace-scope deduction guide
//     `unexpected(E) -> unexpected<E>` in bind_free_operators' namespace walk
//     crashed the Itanium mangler; fixed in the toolchain (guides now mangle)
//     AND the binder now strips guides before the lift.
//   - BINDER-0012: `unexpected() = delete;` was bound as init<> (TU-wide hard
//     error); the binder now filters deleted functions on every path.
//   - TC-0006/TC-0007 blocked ^^tl::expected<void, std::string> from the pack
//     entirely: members_of eagerly instantiated its lazily-ill-formed member
//     bodies (wrong-reject), and can_substitute on swap<OT=void> SEGV'd.
//     Fixed; the void spec now binds, with swap/value gracefully reported
//     not-substitutable-for-void and skipped.
//   - TC-0009 (found by THIS run's first post-fix execution): the four
//     same-headed value() member templates (const&/&/const&&/&&) of an
//     expected<T,E> specialization mangled identically as reflection NTTPs
//     (ODRHash::AddFunctionDecl no-ops in specialization context), so codegen
//     folded the binder's dispatch bodies and value() never bound.
// See corpus/findings/TC-000{6,7,8,9}-*.md and BINDER-0012-*.md.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "extest.h"

namespace nb = nanobind;

NB_MODULE(expected_ext, m) {
    nb::reflect_<^^tl::expected<int, std::string>,
                 ^^tl::expected<std::string, int>,
                 ^^tl::expected<void, std::string>,
                 ^^tl::unexpected<std::string>,
                 ^^tl::unexpected<int>,
                 ^^tl::bad_expected_access<std::string>,
                 ^^extest>(m);
}
