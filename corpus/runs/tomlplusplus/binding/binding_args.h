// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// source_region::path / the source_path_ptr-taking parse_error ctors use
// std::shared_ptr<const std::string>; shared_ptr<T> binding needs T to be a
// bound class, but std::string has a type caster. No other shared_ptr is in
// this surface, so the template is opaque: path / those ctors are skipped,
// begin/end stay bound.
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^toml::node_type, ^^toml::date, ^^toml::time, ^^toml::time_offset,        \
    ^^toml::date_time, ^^toml::source_position, ^^toml::source_region,         \
    ^^toml::parse_error, ^^tomlfix,                                            \
    ^^nanobind::exclude_<^^std::shared_ptr>
