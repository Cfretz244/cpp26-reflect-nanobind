"""Differential test for the cpp-httplib binding (Layer-1 + Layer-3).

The binder binds httplib's Client + value types (Request/Response/Result) + the
Error/StatusCode enums head-on. The bound Client is driven over REAL loopback against the
httptest::EchoServer fixture (a real httplib::Server on a background C++ thread). The native
oracle drives the IDENTICAL client calls against the SAME fixture server and emits every
observable as JSON; the assertions compare the bound module against that ground truth (shared
compiler + shared httplib.h).

Every Get/Delete overload takes a DownloadProgress (std::function) parameter; the default
nullptr is a C++26-unbindable default VALUE and None is not an empty std::function, so the
progress callback is passed explicitly here -- an always-continue `lambda c, t: True`, the
same callback the native oracle passes -- so both sides drive identical calls.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("httplib_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# always-continue progress callback (every Get/Delete + the body Post/Put overloads take one)
PROG = lambda current, total: True


@pytest.fixture(scope="module")
def server():
    srv = m.EchoServer()
    port = srv.start()
    assert port > 0
    yield port
    srv.stop()


@pytest.fixture(scope="module")
def client(server):
    return m.Client("127.0.0.1", server)


# --- Layer 1: differential vs the native oracle ---

def test_get_echo_differential(client):
    r = client.Get("/echo", PROG)
    assert bool(r) is bool(E["get_ok"])
    assert bool(r) is True
    resp = r.value()
    assert resp.status == E["get_status"]
    assert resp.body == E["get_body"]
    assert resp.get_header_value("Content-Type", "", 0) == E["get_ct"]
    assert resp.get_header_value("X-Echo-Method", "", 0) == E["get_echo_method"]
    assert resp.get_header_value("X-Echo-Path", "", 0) == E["get_echo_path"]
    assert r.error().value == E["get_error"]
    assert r.error() == m.Error.Success


def test_get_via_deref_operator(client):
    # operator-> is not bindable; value() is the head-on accessor. operator* -> value().
    r = client.Get("/echo", PROG)
    # value() returns the Response by reference (borrowed); status echoes through.
    assert r.value().status == E["get_status"]


def test_missing_route_404_differential(client):
    r = client.Get("/missing", PROG)
    assert bool(r) is bool(E["missing_ok"])  # a 404 still yields a valid Result
    assert bool(r) is True
    assert r.value().status == E["missing_status"]
    assert r.value().status == E["status_notfound_value"]


def test_teapot_status_differential(client):
    r = client.Get("/teapot", PROG)
    assert r.value().status == E["teapot_status"]
    assert r.value().status == 418
    assert r.value().body == E["teapot_body"]


def test_post_echo_differential(client):
    r = client.Post("/echo", "hello world", "text/plain", PROG)
    assert bool(r) is bool(E["post_ok"])
    resp = r.value()
    assert resp.status == E["post_status"]
    assert resp.status == E["status_created_value"]
    assert resp.body == E["post_body"]
    assert resp.get_header_value("Content-Type", "", 0) == E["post_ct"]
    assert resp.get_header_value("X-Body-Len", "", 0) == E["post_body_len"]


def test_post_json_body_differential(client):
    r = client.Post("/echo", '{"k":42}', "application/json", PROG)
    resp = r.value()
    assert resp.status == E["post2_status"]
    assert resp.body == E["post2_body"]
    assert resp.get_header_value("Content-Type", "", 0) == E["post2_ct"]


def test_put_differential(client):
    r = client.Put("/echo", "payload", "text/plain", PROG)
    assert r.value().status == E["put_status"]
    assert r.value().body == E["put_body"]


def test_delete_differential(client):
    r = client.Delete("/echo", PROG)
    assert bool(r) is bool(E["delete_ok"])
    assert r.value().status == E["delete_status"]
    assert r.value().status == E["status_nocontent_value"]


def test_error_path_closed_port_differential():
    # connect to a closed port -> falsy Result carrying Error::Connection
    dead = m.Client("127.0.0.1", 1)
    r = dead.Get("/x", PROG)
    assert bool(r) is bool(E["closed_ok"])
    assert bool(r) is False
    assert r.error().value == E["closed_error"]
    assert r.error() == m.Error.Connection


def test_enum_values_differential():
    assert m.Error.Success.value == E["err_success_value"]
    assert m.Error.Connection.value == E["err_connection_value"]
    assert m.StatusCode.OK_200.value == E["status_ok_value"]
    assert m.StatusCode.Created_201.value == E["status_created_value"]
    assert m.StatusCode.NoContent_204.value == E["status_nocontent_value"]
    assert m.StatusCode.NotFound_404.value == E["status_notfound_value"]


# --- Layer 3: structural invariants ---

def test_surface_present():
    for verb in ("Get", "Post", "Put", "Patch", "Delete", "Head", "Options"):
        assert hasattr(m.Client, verb), verb
    for meth in ("is_valid", "host", "port"):
        assert hasattr(m.Client, meth), meth
    for meth in ("status", "body", "reason", "get_header_value", "has_header",
                 "set_header", "set_content"):
        assert hasattr(m.Response, meth), meth
    for meth in ("method", "path", "body", "get_header_value", "get_param_value",
                 "has_param"):
        assert hasattr(m.Request, meth), meth
    for meth in ("error", "value", "has_request_header"):
        assert hasattr(m.Result, meth), meth


def test_result_bool_dunder(client):
    # operator bool() -> __bool__: a successful Result is truthy, a closed-port one falsy.
    ok = client.Get("/echo", PROG)
    assert bool(ok) is True
    dead = m.Client("127.0.0.1", 1)
    bad = dead.Get("/x", PROG)
    assert bool(bad) is False


def test_response_is_value_type(client):
    # Response is a distinct bound class; value() yields an instance of it.
    r = client.Get("/echo", PROG)
    assert isinstance(r.value(), m.Response)


def test_error_enum_completeness():
    # head + a couple of named members present with declared ordinals.
    assert m.Error.Success.value == 0
    assert m.Error.Unknown.value == 1
    assert m.Error.Connection.value == 2
    # StatusCode carries the standard codes.
    assert m.StatusCode.OK_200.value == 200
    assert m.StatusCode.NotFound_404.value == 404


def test_client_host_port(server):
    c = m.Client("127.0.0.1", server)
    assert c.host() == "127.0.0.1"
    assert c.port() == server
