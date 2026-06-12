// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The to_string<T> instantiations bind under template-spec CamelCase names
// with an enable_if-NTTP suffix (BINDER-0003, open cosmetic).
// The enum listings are replaced by a match_ marker: every namespace-level
// enum in fmt proper (the prior three from <fmt/color.h> plus
// presentation_type/align/sign/arg_id_kind from base.h and range_format from
// ranges.h), with fmt::detail gated out by the predicate -- detail enums like
// type/state/dragon are real declarations (not aliases), so in_namespace_
// fences them reliably. The anonymous `enum { inline_buffer_size = 500 }` in
// fmt proper is matcher-accepted and then gracefully skipped by reflect_enum
// (no identifier), a deliberate exercise of that path.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^mirrorbind::match_<^^fmt,                                                  \
        mirrorbind::all_of_<mirrorbind::is_enum_,                                 \
            mirrorbind::not_<mirrorbind::in_namespace_<^^fmt::detail>>>>,         \
    ^^fmt::format_int,                                                         \
    ^^fmt::to_string<int>, ^^fmt::to_string<double>,                           \
    ^^fmt::to_string<long long>,                                               \
    ^^fmttest
