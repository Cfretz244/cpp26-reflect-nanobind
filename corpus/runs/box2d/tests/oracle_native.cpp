// Native oracle for the box2d run. Drives a deterministic physics scenario through
// box2d natively and prints ONE JSON object on stdout. test_bindings.py drives the
// SAME scenario through the Python module and asserts bit-for-bit equality (both sides
// link the identical compiled archive, so the floats are byte-identical -- compared
// via %.9g formatted strings).
//
// Scenario: a dynamic 1x1 box dropped from (0, 6) onto a static ground floor at y=0
// under gravity (0, -10), stepped 90 times at dt=1/60 with 8 velocity / 3 position
// iterations. We sample position/velocity/angle of the falling body at a few steps and
// at rest, plus the world body count and the body's sleep state at the end.

#include <box2d/box2d.h>
#include <cstdio>
#include <vector>

static void g(const char* key, float v, bool last = false) {
    std::printf("  \"%s\": \"%.9g\"%s\n", key, v, last ? "" : ",");
}

int main() {
    b2Vec2 gravity(0.0f, -10.0f);
    b2World world(gravity);

    // Static ground: a wide thin box at the origin.
    b2BodyDef groundDef;
    groundDef.position.Set(0.0f, 0.0f);
    b2Body* ground = world.CreateBody(&groundDef);
    b2PolygonShape groundBox;
    groundBox.SetAsBox(50.0f, 1.0f);
    ground->CreateFixture(&groundBox, 0.0f);

    // Dynamic falling box.
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0.0f, 6.0f);
    b2Body* body = world.CreateBody(&bodyDef);
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(0.5f, 0.5f);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;
    body->CreateFixture(&fixtureDef);

    const float dt = 1.0f / 60.0f;
    const int velIters = 8, posIters = 3;

    // Sample positions at a few step counts during the fall.
    std::vector<int> samples = {1, 10, 30, 60, 90};
    size_t si = 0;

    std::printf("{\n");
    g("gravity_y", world.GetGravity().y);
    g("body_count", (float)world.GetBodyCount());
    g("ground_mass", ground->GetMass());
    g("body_mass", body->GetMass());

    for (int step = 1; step <= 90; ++step) {
        world.Step(dt, velIters, posIters);
        if (si < samples.size() && step == samples[si]) {
            char k[64];
            std::snprintf(k, sizeof k, "px_%d", step);  g(k, body->GetPosition().x);
            std::snprintf(k, sizeof k, "py_%d", step);  g(k, body->GetPosition().y);
            std::snprintf(k, sizeof k, "vy_%d", step);  g(k, body->GetLinearVelocity().y);
            std::snprintf(k, sizeof k, "angle_%d", step); g(k, body->GetAngle());
            ++si;
        }
    }

    // At-rest state.
    g("rest_px", body->GetPosition().x);
    g("rest_py", body->GetPosition().y);
    g("rest_vx", body->GetLinearVelocity().x);
    g("rest_vy", body->GetLinearVelocity().y);
    g("rest_angle", body->GetAngle());
    g("rest_awake", body->IsAwake() ? 1.0f : 0.0f);
    g("body_type", (float)body->GetType());
    g("ground_type", (float)ground->GetType());

    // A b2Vec2 operator/value-type cross-check independent of the sim.
    b2Vec2 a(3.0f, 4.0f);
    g("vec_len", a.Length());
    b2Vec2 sum = a + b2Vec2(1.0f, 1.0f);
    g("vec_sum_x", sum.x);
    g("vec_sum_y", sum.y);
    g("vec_eq", (a == b2Vec2(3.0f, 4.0f)) ? 1.0f : 0.0f, true);
    std::printf("}\n");
    return 0;
}
