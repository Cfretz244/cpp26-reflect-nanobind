// Single-stage bindings for Abseil (20250814.2) — the "special case" run: binding fully-specialized
// concrete data structures directly from a real, NON-header-only library.
//
// Subset (Gate 2): two specializations of absl::InlinedVector (a small-buffer-optimized vector) —
// <int,4> and <double,2>. The binder reflects each specialization head-on (no fixture wrappers):
// push_back/pop_back/at/operator[]/front/back/size/capacity/clear/empty/reserve/resize/erase/insert
// all bind. The two distinct specializations bind to distinct Python types (InlinedVectorInt4,
// InlinedVectorDouble2), exercising the binder on multiple complex specializations of one template.
//
// Abseil is NOT header-only: InlinedVector::at() references absl::base_internal::ThrowStdOutOfRange,
// defined in absl/base/internal/throw_delegate.cc. That .cc is compiled and linked in via
// meta.extra_sources (NB_EXTRA_SOURCES) — the minimal "link the library" path (see build_module.sh).
//
// Findings deferred out of this subset (real binder-coverage items surfaced by Abseil, written up
// under findings/): absl::FixedArray's internal zero-length-array storage member (BINDER-0006) and
// absl::int128's free operator<<(std::ostream&, ...) which the binder tries to bind as a dunder,
// instantiating an incomplete std::ostream caster (BINDER-0007).
#include <nanobind/nb_reflect.h>

#include "absl/container/inlined_vector.h"

namespace nb = nanobind;

NB_MODULE(abseil_ext, m) {
    nb::reflect_<^^absl::InlinedVector<int, 4>, ^^absl::InlinedVector<double, 2>>(m);
}
