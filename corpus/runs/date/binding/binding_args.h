// The reflect_ pack for the date run, defined ONCE for every backend consumer
// (P2996-only). Mirrors the reflect_args in meta.toml.
// One match_ marker replaces the seven explicit listings (same classes, same
// names -- the unchanged legacy suite is the equivalence proof) and extends
// the selection to the whole indexed/last calendrical family: is_class_
// keeps free functions out by construction, and the name globs are
// normalization-safe (date's days/months/years aliases dealias to
// std::chrono::duration, whose template identifier "duration" no glob
// matches; date::detail holds nothing the globs touch). last_spec is
// included deliberately: it makes the last DSL drivable from Python
// (weekday[last_spec] -> weekday_last, month/last_spec -> month_day_last,
// year/month/last_spec -> year_month_day_last) through the already-bound
// free operator/ overloads and weekday's __getitem__ overload pair.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^mirrorbind::match_<^^date, \
        mirrorbind::all_of_<mirrorbind::is_class_, \
            mirrorbind::any_of_<mirrorbind::named_<"day">, \
                              mirrorbind::named_<"month*">, \
                              mirrorbind::named_<"year*">, \
                              mirrorbind::named_<"weekday*">, \
                              mirrorbind::named_<"last_spec">>>>, \
    ^^datefix
