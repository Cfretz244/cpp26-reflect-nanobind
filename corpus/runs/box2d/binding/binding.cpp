// Single-stage bindings for box2d v2.4.2 (Tier 4: a real rigid-body physics engine
// bound head-on -- math value types, the world/body/fixture object graph, plain option
// structs, and concrete shape classes).
//
// box2d's public types live in the GLOBAL namespace (no enclosing namespace to walk),
// so every concrete type is listed explicitly in the reflect_ pack.
//
// Bound head-on:
//   b2Vec2          -- the 2D vector value type: x/y fields, operators (==,!=,+,-,unary-,
//                      +=,-=,*=), Length/LengthSquared/Normalize/Skew/Dot helpers.
//   b2World         -- the simulation root: ctor(const b2Vec2& gravity), Step,
//                      CreateBody/DestroyBody, GetBodyCount, Get/SetGravity, sleep flags,
//                      IsLocked, GetBodyList (raw b2Body* returns BORROW -- world-owned).
//   b2Body          -- the rigid body handle: Get/SetTransform, GetPosition/GetAngle,
//                      Get/SetLinearVelocity, GetMass, type/awake/bullet accessors,
//                      CreateFixture, GetWorldPoint, etc. Created via b2World::CreateBody.
//   b2BodyDef       -- the plain body-definition option struct + b2BodyType enum.
//   b2FixtureDef    -- the fixture-definition option struct (shape ptr + material props).
//   b2Filter        -- the collision-filter option struct.
//   b2Fixture       -- the fixture handle: GetDensity/GetFriction/IsSensor/GetType/GetBody.
//   b2Shape         -- the abstract shape base (Type enum; no Python ctor -- abstract).
//   b2PolygonShape  -- SetAsBox + Validate; the box collider.
//   b2CircleShape   -- m_p position + radius; the circle collider.
//   b2MassData/b2Transform/b2Rot -- small value structs reachable from the accessors.
//
// Deliberately excluded (opaque on every path): the internal facade types reachable
// from b2World's accessors that have no Python meaning and drag in deep internals --
// b2ContactManager (holds a b2BroadPhase/b2DynamicTree), b2Profile (timing struct),
// and the b2_draw debug-draw color type. Everything else the methods mention is a
// forward-declared class (b2Joint/b2Contact/b2*Listener/b2*Callback/b2Draw/...), which
// the binder makes opaque automatically and skips the mentioning methods.

#include <nanobind/nb_reflect.h>

#include <box2d/box2d.h>

namespace nb = nanobind;

NB_MODULE(box2d_ext, m) {
    nb::reflect_<
        // --- math value types ---
        ^^b2Vec2, ^^b2Vec3, ^^b2Mat22, ^^b2Rot, ^^b2Transform, ^^b2MassData,
        // --- the object graph ---
        ^^b2World, ^^b2Body, ^^b2Fixture,
        // --- plain option structs + enum ---
        ^^b2BodyDef, ^^b2BodyType, ^^b2FixtureDef, ^^b2Filter,
        // --- shapes ---
        ^^b2Shape, ^^b2PolygonShape, ^^b2CircleShape,
        // --- exclusions: internal facades with no Python meaning ---
        ^^nb::exclude_<^^b2ContactManager, ^^b2Profile, ^^b2Color,
                       ^^b2BlockAllocator, ^^b2BroadPhase>
    >(m);
}
