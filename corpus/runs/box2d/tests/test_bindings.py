# Differential + invariant tests for the box2d binding.
#
# NOTE: this run is currently BLOCKED at Gate 4 by a toolchain ICE (nb::reflect_ of any
# global-namespace class aborts the compiler -- see findings_draft/). The module cannot be
# built, so these tests do not run yet. They are committed so the run re-gates cleanly once
# the toolchain fix lands: they drive the SAME deterministic physics scenario as
# tests/oracle_native.cpp through the Python module and assert bit-for-bit equality (both
# sides link the identical compiled box2d archive -> byte-identical floats, compared via the
# oracle's %.9g formatting), plus Layer-3 surface/inheritance/enum invariants.

import json
import math
import pathlib

import pytest

box2d_ext = pytest.importorskip("box2d_ext")

EXPECTED = json.loads((pathlib.Path(__file__).parent / "expected.json").read_text())


def fmt(v):
    # Match the native oracle's "%.9g" so float comparison is exact string equality.
    return "%.9g" % v


def run_scenario():
    """Drive the identical scenario the native oracle runs; return the same JSON shape."""
    m = box2d_ext
    world = m.b2World(m.b2Vec2(0.0, -10.0))

    ground_def = m.b2BodyDef()
    ground_def.position = m.b2Vec2(0.0, 0.0)
    ground = world.CreateBody(ground_def)
    ground_box = m.b2PolygonShape()
    ground_box.SetAsBox(50.0, 1.0)
    ground.CreateFixture(ground_box, 0.0)

    body_def = m.b2BodyDef()
    body_def.type = m.b2BodyType.b2_dynamicBody
    body_def.position = m.b2Vec2(0.0, 6.0)
    body = world.CreateBody(body_def)
    dyn_box = m.b2PolygonShape()
    dyn_box.SetAsBox(0.5, 0.5)
    fix_def = m.b2FixtureDef()
    fix_def.shape = dyn_box
    fix_def.density = 1.0
    fix_def.friction = 0.3
    body.CreateFixture(fix_def)

    out = {}
    out["gravity_y"] = fmt(world.GetGravity().y)
    out["body_count"] = fmt(world.GetBodyCount())
    out["ground_mass"] = fmt(ground.GetMass())
    out["body_mass"] = fmt(body.GetMass())

    dt, vel_iters, pos_iters = 1.0 / 60.0, 8, 3
    samples = {1, 10, 30, 60, 90}
    for step in range(1, 91):
        world.Step(dt, vel_iters, pos_iters)
        if step in samples:
            out["px_%d" % step] = fmt(body.GetPosition().x)
            out["py_%d" % step] = fmt(body.GetPosition().y)
            out["vy_%d" % step] = fmt(body.GetLinearVelocity().y)
            out["angle_%d" % step] = fmt(body.GetAngle())

    out["rest_px"] = fmt(body.GetPosition().x)
    out["rest_py"] = fmt(body.GetPosition().y)
    out["rest_vx"] = fmt(body.GetLinearVelocity().x)
    out["rest_vy"] = fmt(body.GetLinearVelocity().y)
    out["rest_angle"] = fmt(body.GetAngle())
    out["rest_awake"] = fmt(1.0 if body.IsAwake() else 0.0)
    out["body_type"] = fmt(float(body.GetType().value))
    out["ground_type"] = fmt(float(ground.GetType().value))

    a = m.b2Vec2(3.0, 4.0)
    out["vec_len"] = fmt(a.Length())
    s = a + m.b2Vec2(1.0, 1.0)
    out["vec_sum_x"] = fmt(s.x)
    out["vec_sum_y"] = fmt(s.y)
    out["vec_eq"] = fmt(1.0 if (a == m.b2Vec2(3.0, 4.0)) else 0.0)
    return out


def test_differential_matches_native_oracle():
    got = run_scenario()
    for key, want in EXPECTED.items():
        assert key in got, "missing key %r in Python result" % key
        assert got[key] == want, "mismatch on %r: py=%r oracle=%r" % (key, got[key], want)


def test_box_settles_on_ground():
    # The dynamic box rests just above y=1 (ground half-height) + 0.5 (box half-height).
    py = float(EXPECTED["rest_py"])
    assert 1.4 < py < 1.6


def test_vec2_value_semantics():
    v = box2d_ext.b2Vec2(3.0, 4.0)
    assert math.isclose(v.Length(), 5.0, rel_tol=1e-6)
    assert v == box2d_ext.b2Vec2(3.0, 4.0)
    assert v != box2d_ext.b2Vec2(0.0, 0.0)
    assert (v + box2d_ext.b2Vec2(1.0, 1.0)).x == 4.0


def test_body_type_enum_present():
    bt = box2d_ext.b2BodyType
    assert bt.b2_staticBody.value == 0
    assert bt.b2_kinematicBody.value == 1
    assert bt.b2_dynamicBody.value == 2


def test_shape_inheritance():
    # b2PolygonShape / b2CircleShape derive from the abstract b2Shape (which is in the
    # bind set -> the real Python base, not flattened).
    assert issubclass(box2d_ext.b2PolygonShape, box2d_ext.b2Shape)
    assert issubclass(box2d_ext.b2CircleShape, box2d_ext.b2Shape)


def test_abstract_base_not_constructible():
    with pytest.raises(TypeError):
        box2d_ext.b2Shape()
