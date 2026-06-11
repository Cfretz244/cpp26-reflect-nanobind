// The reflect_ pack for the expected run, defined ONCE for every backend
// consumer (P2996-only). Mirrors the reflect_args in meta.toml.
//   tl::expected<T,E> bound head-on as three real specializations (int/string,
//   the role-swapped string/int, and the no-value void/string channel),
//   tl::unexpected<std::string>/<int> for their real non-template ctors,
//   tl::bad_expected_access<std::string> as a class, and the extest fixture.
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^tl::expected<int, std::string>, \
    ^^tl::expected<std::string, int>, \
    ^^tl::expected<void, std::string>, \
    ^^tl::unexpected<std::string>, \
    ^^tl::unexpected<int>, \
    ^^tl::bad_expected_access<std::string>, \
    ^^extest
