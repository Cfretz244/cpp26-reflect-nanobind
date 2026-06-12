// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
//
// box2d's public types are all GLOBAL-namespace classes, so every concrete
// type is listed explicitly. exclude_ makes the internal facade types opaque
// on every path (BINDER-0014): b2ContactManager / b2Profile / b2Color /
// b2BlockAllocator / b2BroadPhase have no Python meaning and would drag in deep
// internals; the methods mentioning them (and the forward-declared
// b2Joint/b2Contact/b2*Listener/... the binder makes opaque automatically) are
// skipped. single_stage -- Python does not override box2d virtuals -- so no
// trampoline marker is appended.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    /* --- math value types --- */                                            \
    ^^b2Vec2, ^^b2Vec3, ^^b2Mat22, ^^b2Rot, ^^b2Transform, ^^b2MassData,      \
    /* --- the object graph --- */                                            \
    ^^b2World, ^^b2Body, ^^b2Fixture,                                         \
    /* --- plain option structs + enum --- */                                 \
    ^^b2BodyDef, ^^b2BodyType, ^^b2FixtureDef, ^^b2Filter,                     \
    /* --- shapes --- */                                                      \
    ^^b2Shape, ^^b2PolygonShape, ^^b2CircleShape,                             \
    /* --- exclusions: internal facades with no Python meaning --- */         \
    ^^mirrorbind::exclude_<^^b2ContactManager, ^^b2Profile, ^^b2Color,          \
                         ^^b2BlockAllocator, ^^b2BroadPhase>
