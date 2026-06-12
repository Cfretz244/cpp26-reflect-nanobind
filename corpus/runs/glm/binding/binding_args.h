// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"

// The original explicit subset (the vec3/vec4 aliases) plus the matcher-API
// demonstration: an instantiate_ grid minting vec<L, T, packed_highp> for
// L in {2,3,4} x T in {float, double} -- six specializations from one rule,
// where the alias route could only name the predefined typedefs. The two
// routes overlap on float vec3/vec4: binding is idempotent and the spec's
// own CamelCase name wins on both routes, so the overlap is benign.
#define CORPUS_REFLECT_ARGS                                                    \
    ^^glm::vec3, ^^glm::vec4,                                                  \
    ^^mirrorbind::instantiate_<^^glm::vec,                                       \
        mirrorbind::product_<                                                    \
            mirrorbind::set_<mirrorbind::val_<glm::length_t(2)>,                   \
                           mirrorbind::val_<glm::length_t(3)>,                   \
                           mirrorbind::val_<glm::length_t(4)>>,                  \
            mirrorbind::set_<^^float, ^^double>,                                 \
            mirrorbind::set_<mirrorbind::val_<glm::qualifier::packed_highp>>>>
