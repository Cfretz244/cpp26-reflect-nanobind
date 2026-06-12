// The reflect_ pack for the Abseil hash-container run, defined ONCE for every
// backend consumer (binding.cpp's NB_MODULE and gen_emit.cpp's generator).
// P2996-only.
//
// flat_hash_map/flat_hash_set/node_hash_map are bound HEAD-ON (the BINDER-0008
// payoff): reachability discovery keeps the container_internal policy explosion
// out; only signature-reachable internals (node_handle, the raw_hash_map/
// raw_hash_set ancestry that becomes the Python base chain) come along.
// Default-instantiable member templates render the heterogeneous query surface
// (contains/count/find/erase/at/equal_range, operator[] -> __getitem__) as
// qualified self.::absl::...::raw_hash_map<...>::template <op><>(...) calls.
// hmtest supplies value-setting population (put/add).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^absl::flat_hash_map<int, std::string>,                                   \
    ^^absl::flat_hash_set<int>,                                                \
    ^^absl::node_hash_map<int, std::string>,                                   \
    ^^hmtest
