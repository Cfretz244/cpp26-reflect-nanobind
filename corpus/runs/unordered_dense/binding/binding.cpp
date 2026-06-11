// Binding TU for the ankerl::unordered_dense run: map<int,std::string> and
// set<int> bound head-on (the real public alias-template specializations, which
// resolve to detail::table specs). The query/mutation surface -- contains/count/
// at/erase/operator[] -> __getitem__/size/empty/clear/bucket_count/load_factor --
// binds directly off the spec; the udtest fixture supplies only value-setting
// population (insert returns pair<iterator,bool>, no Python representation).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>

#include "binding_args.h"

namespace nb = nanobind;

NB_MODULE(unordered_dense_ext, m) {
    nb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
