// Binding TU for the immer run (Tier 4: persistent/immutable containers).
//
// immer's whole point is value semantics with structural sharing: push_back/set/insert
// RETURN A NEW container by value, leaving the receiver untouched. We bind the library's
// own concrete specializations head-on -- immer::vector<int>, immer::vector<std::string>,
// immer::map<int,int>, immer::set<int> -- listed EXPLICITLY in the reflect_<...> pack
// (a spec's own template args are not signature-reachable). The bind set + the
// immer::detail exclusion marker live in binding_args.h (shared with the emit lane).
//
// This is the BINDER-0026 run: reflected constructors construct with PARENS via
// reflect_init (nb_paren_init.h) -- immer::vector's (size_type, T) fill ctor would
// otherwise have its braced init Type{n, v} hijacked toward the initializer_list<T>
// ctor (a hard narrowing error for vector<int>).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/pair.h>

#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(immer_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
