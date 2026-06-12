// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
#pragma once
#include <nanobind/nb_reflect.h>
#include "binding_includes.h"

// The original explicit subset (the vec3/vec4 aliases) plus the matcher-API
// demonstration: an instantiate_ grid minting vec<L, T, packed_highp> for
// L in {2,3,4} x T in {float, double} -- six specializations from one rule,
// where the alias route could only name the predefined typedefs. The two
// routes overlap on float vec3/vec4: binding is idempotent and the spec's
// own CamelCase name wins on both routes, so the overlap is benign.
#define CORPUS_REFLECT_ARGS                                                    \
    ^^glm::vec3, ^^glm::vec4,                                                  \
    ^^nanobind::instantiate_<^^glm::vec,                                       \
        nanobind::product_<                                                    \
            nanobind::set_<nanobind::val_<glm::length_t(2)>,                   \
                           nanobind::val_<glm::length_t(3)>,                   \
                           nanobind::val_<glm::length_t(4)>>,                  \
            nanobind::set_<^^float, ^^double>,                                 \
            nanobind::set_<nanobind::val_<glm::qualifier::packed_highp>>>>
