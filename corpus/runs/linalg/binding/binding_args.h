// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The single vec<float,3> listing grows into the full instantiate_ grid
// story, exercising BOTH target forms:
//  - the matcher-targeted form (instantiate_<^^named_<"vec">, ...>) sweeps
//    the pack's namespace roots for class templates named vec -- the
//    match_<^^linalg, ...> marker supplies ^^linalg as that root (and binds
//    identity_t, real API, on the way); the product grid mints
//    vec<{float,double,int}, {2,3,4}> (M is a plain int NTTP, so bare
//    val_<2> is exactly typed);
//  - the direct class-template form mints the float mat<2..4 x 2..4> grid
//    (columns are vec<float,M>, row() returns vec<float,N> -- both already
//    in the vec grid, so discovery is closed).
// The match_ matcher is deliberately the EXACT name identity_t: linalg's
// ~110 typedefs in linalg::aliases all DEALIAS to vec/mat themselves under
// matcher normalization (named_/in_namespace_ cannot tell float3 from vec),
// so any broad is_class_-style matcher here would detonate the bind set
// into the full 64-mat alias surface. Exact-name matching is structurally
// safe from that.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^linalg::vec<float, 3>,                                                   \
    ^^mirrorbind::match_<^^linalg, mirrorbind::named_<"identity_t">>,            \
    ^^mirrorbind::instantiate_<^^mirrorbind::named_<"vec">,                       \
        mirrorbind::product_<                                                    \
            mirrorbind::set_<^^float, ^^double, ^^int>,                           \
            mirrorbind::set_<mirrorbind::val_<2>, mirrorbind::val_<3>,             \
                           mirrorbind::val_<4>>>>,                              \
    ^^mirrorbind::instantiate_<^^linalg::mat,                                    \
        mirrorbind::product_<                                                    \
            mirrorbind::set_<^^float>,                                           \
            mirrorbind::set_<mirrorbind::val_<2>, mirrorbind::val_<3>,             \
                           mirrorbind::val_<4>>,                                \
            mirrorbind::set_<mirrorbind::val_<2>, mirrorbind::val_<3>,             \
                           mirrorbind::val_<4>>>>
