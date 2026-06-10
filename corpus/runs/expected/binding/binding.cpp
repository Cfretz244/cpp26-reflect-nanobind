// Single-stage bindings for TartanLlama/expected v1.3.1 (Tier 3: error-channel
// vocabulary type). Two real expected<T,E> specializations bound head-on --
// int/std::string value-or-error and the role-swapped std::string/int.
// tl::unexpected<std::string>/<int> are listed for their REAL non-template ctors
// (constructible from a Python str/int directly); tl::bad_expected_access<std::string>
// binds as a class (ctor + error()/what()); the extest fixtures supply factories (the
// converting ctors are templates) and thin wrappers over the all-template operator== /
// value_or / monadic fronts.
//
// KNOWN-FAILING (Gate 4, outcome B) at the toolchain+binder commits pinned by this
// repo, two layers deep:
//   1. RECORDED failure (codegen ICE, TC-0008): tl declares the deduction guide
//      `unexpected(E) -> unexpected<E>` at namespace scope; bind_free_operators'
//      define_static_array(members_of(^^tl, ...)) walk puts its reflection in
//      mangled-name position and the Itanium mangler aborts ("Can't mangle a
//      deduction guide name!", ItaniumMangle.cpp:1774). No consumer-side dodge:
//      binding ANY tl class triggers the namespace walk.
//   2. Behind it (visible at -fsyntax-only, BINDER-0012): tl::unexpected<E> declares
//      `unexpected() = delete;` and the binder's constructor pass has no is_deleted
//      filter, so cls.def(init<>()) hard-errors ("call to deleted constructor").
//      The unexpected<E> specs cannot be left out of the pack to dodge it:
//      expected<T,E>'s default-instantiable operator=(unexpected<G>&&) member
//      template makes them signature-reachable, so discovery re-adds them.
// See corpus/findings/TC-0008-*.md and BINDER-0012-*.md.
//
// DELIBERATELY DROPPED: ^^tl::expected<void, std::string> -- reflecting it
// wrong-rejects (TC-0007) and can_substitute on its swap member template SEGVs the
// compiler (TC-0006); see corpus/findings/TC-0006-*.md / TC-0007-*.md.
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "extest.h"

namespace nb = nanobind;

NB_MODULE(expected_ext, m) {
    nb::reflect_<^^tl::expected<int, std::string>,
                 ^^tl::expected<std::string, int>,
                 ^^tl::unexpected<std::string>,
                 ^^tl::unexpected<int>,
                 ^^tl::bad_expected_access<std::string>,
                 ^^extest>(m);
}
