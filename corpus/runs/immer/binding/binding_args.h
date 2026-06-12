// The reflect_ pack for the immer run, defined ONCE for every backend consumer
// (constexpr `mb::reflect_<...>` and emit `mb::write_bindings<...>`). P2996-only.
//
// immer's persistent containers are bound head-on as four real concrete
// specializations listed explicitly (a spec's own template args are not
// signature-reachable): immer::vector<int>, immer::vector<std::string>,
// immer::map<int,int>, immer::set<int>. The whole immer::detail implementation
// namespace (rbts:: rbtree/node/iterator, hamts:: champ/node/iterator) is
// reachable through begin()/end()/impl() but diverges hard; it is excluded
// transitively via an ^^mb::exclude_<...> marker assembled by the consteval
// helper below.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"

namespace mb = mirrorbind;

namespace nbcorpus_immer {

namespace sm = std::meta;

// The exclusion set, assembled as an ^^mb::exclude_<...> marker:
//   the whole immer::detail namespace (transitive). immer's entire
//   implementation lives there -- rbts:: rbtree/node/iterator, hamts::
//   champ/node/iterator -- reachable from the public containers'
//   begin()/end()/impl(). Reflecting into it diverges hard: the iterators
//   derive from immer::detail::iterator_facade whose operator overloads
//   re-check enable_if default template args naming its PROTECTED
//   is_random_access/is_bidirectional from a no-access context, and the
//   internal node types expose static factories whose pointer signatures trip
//   the binder's static-method lambda. None of it is part of this run's
//   persistent-value differential.
consteval sm::info immer_excluded_marker() {
    std::vector<sm::info> args;
    args.push_back(sm::reflect_constant(^^immer::detail));
    return sm::substitute(^^mirrorbind::exclude_, args);
}

}  // namespace nbcorpus_immer

#define CORPUS_REFLECT_ARGS                                                    \
    ^^immer::vector<int>, ^^immer::vector<std::string>,                        \
    ^^immer::map<int, int>, ^^immer::set<int>,                                 \
    ::nbcorpus_immer::immer_excluded_marker()
