// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// source_region::path / the source_path_ptr-taking parse_error ctors use
// std::shared_ptr<const std::string>; shared_ptr<T> binding needs T to be a
// bound class, but std::string has a type caster. No other shared_ptr is in
// this surface, so the template is opaque: path / those ctors are skipped,
// begin/end stay bound.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
// toml::impl holds the container facade internals -- table/array iterators and
// the table_proxy_pair / table_init_pair view aggregates. Recursive plain-class
// discovery reaches them through table/array's public begin()/end()/iteration,
// but they wrap std iterators over move-only std::unique_ptr<node> and bind no
// useful Python surface (an internal iterator is not a Python type). Excise the
// namespace: the methods that mention them are dropped, the clean public surface
// (node / table / array / value / node_view) stays.
#define CORPUS_REFLECT_ARGS                                                    \
    ^^toml::node_type, ^^toml::date, ^^toml::time, ^^toml::time_offset,        \
    ^^toml::date_time, ^^toml::source_position, ^^toml::source_region,         \
    ^^toml::parse_error, ^^tomlfix,                                            \
    ^^mirrorbind::exclude_<^^std::shared_ptr, ^^toml::impl>
