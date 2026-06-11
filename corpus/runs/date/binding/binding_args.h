// The reflect_ pack for the date run, defined ONCE for every backend consumer
// (P2996-only). Mirrors the reflect_args in meta.toml.
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^date::year, ^^date::month, ^^date::day, \
    ^^date::weekday, \
    ^^date::year_month, ^^date::month_day, \
    ^^date::year_month_day, \
    ^^datefix
