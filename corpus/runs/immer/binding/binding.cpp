// Single-stage bindings for immer v0.9.1 (Tier 4: persistent/immutable containers).
//
// immer's whole point is value semantics with structural sharing: push_back/set/insert
// RETURN A NEW container by value, leaving the receiver untouched. We bind the library's
// own concrete specializations head-on:
//   immer::vector<int>, immer::vector<std::string>, immer::map<int,int>, immer::set<int>
// listed EXPLICITLY in the reflect_<...> pack (a spec's own template args are not
// signature-reachable).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/pair.h>

#include <immer/vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>

#include <string>

namespace nb = nanobind;
namespace sm = std::meta;

namespace {

// The exclusion set, assembled as an ^^nb::exclude_<...> marker:
//   1. the whole immer::detail namespace (transitive). immer's entire implementation
//      lives there -- rbts:: rbtree/node/iterator, hamts:: champ/node/iterator -- reachable
//      from the public containers' begin()/end()/impl(). Reflecting into it diverges hard:
//      the iterators derive from immer::detail::iterator_facade whose operator overloads
//      re-check enable_if default template args naming its PROTECTED is_random_access/
//      is_bidirectional from a no-access context, and the internal node types expose static
//      factories whose pointer signatures trip the binder's static-method lambda. None of it
//      is part of this run's persistent-value differential.
//   2. the 2-argument fill constructor vector(size_type n, T v={}) of each bound vector spec.
//      The binder emits nb::init<size_t, T>, but nanobind's init<>::execute brace-inits
//      Type{args...}, which a class also carrying an initializer_list<T> ctor (immer::vector
//      has one) hijacks toward that list -- a hard narrowing error for vector<int>. See
//      findings_draft/binder-init-braces-prefer-initializer-list.md. The fill ctor is not
//      needed for the differential (vectors are built persistently from empty()).
consteval sm::info immer_excluded_marker() {
    std::vector<sm::info> args;
    auto add = [&](sm::info e) { args.push_back(sm::reflect_constant(e)); };

    add(^^immer::detail);

    for (sm::info V : {^^immer::vector<int>, ^^immer::vector<std::string>}) {
        sm::info value_t = sm::template_arguments_of(V)[0];
        for (auto m : sm::members_of(V, sm::access_context::unchecked())) {
            if (!sm::is_constructor(m) || sm::is_template(m) || sm::is_deleted(m))
                continue;
            auto ps = sm::parameters_of(m);
            // vector(size_type n, T v = {}): exactly two params, the second is T.
            if (ps.size() == 2 &&
                sm::remove_cvref(sm::type_of(ps[1])) == sm::remove_cvref(value_t))
                add(m);
        }
    }

    return sm::substitute(^^nb::exclude_, args);
}

}  // namespace

NB_MODULE(immer_ext, m) {
    nb::reflect_<^^immer::vector<int>,
                 ^^immer::vector<std::string>,
                 ^^immer::map<int, int>,
                 ^^immer::set<int>,
                 immer_excluded_marker()>(m);
}
