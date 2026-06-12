"""Differential test for the spdlog binding (Layer-1 + Layer-3).

spdlog::logger is bound head-on; the logtest CaptureSink fixture makes its sink output
observable as text. oracle_native.cpp drives the EXACT same scenario (same logger name,
pattern, messages, level filtering, clone) through native spdlog and emits every
observable; the assertions compare the bound module byte-for-byte against that ground
truth. Layer 3 checks the Tier-3 themes structurally: the sink inheritance chain
(stdout_sink_mt -> stdout_sink_base<console_mutex> -> sink) and shared_ptr round-trips.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("spdlog_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

L = m.level_enum

# The concrete stdout sink binds under its spec-derived name (BINDER-0003 cosmetic):
# stdout_sink<details::console_mutex> -> stdout_sinkConsole_mutex.
StdoutSink = m.stdout_sinkConsole_mutex


def scenario():
    """The shared logging scenario -- mirrors oracle_native.cpp verbatim."""
    cap = m.CaptureSink()
    lg = m.logger("pylog", cap.sink())
    lg.set_pattern("[%n] [%l] %v", m.pattern_time_type.local)
    lg.set_level(L.debug)
    lg.log(L.info, "hello world")
    lg.log(L.trace, "filtered out")     # below debug => dropped
    lg.log(L.err, "boom")
    lg.flush()
    return cap, lg


def test_captured_log_text_differential():
    cap, _ = scenario()
    assert cap.text() == E["cap_text"]


def test_logger_state_differential():
    _, lg = scenario()
    assert lg.name() == E["logger_name"]
    assert lg.level().value == E["logger_level"]
    assert lg.flush_level().value == E["flush_level_default"]
    assert lg.should_log(L.trace) == E["should_log_trace"]
    assert lg.should_log(L.debug) == E["should_log_debug"]
    assert lg.should_log(L.err) == E["should_log_err"]
    assert len(lg.sinks()) == E["n_sinks"]


def test_clone_shares_sinks_differential():
    cap, lg = scenario()
    cl = lg.clone("worker")             # shared_ptr<logger> return
    cap.clear()
    cl.log(L.warn, "from clone")
    cl.flush()
    assert cl.name() == E["clone_name"]
    assert cap.text() == E["clone_text"]


def test_level_enum_differential():
    assert L.trace.value == E["lvl_trace"]
    assert L.debug.value == E["lvl_debug"]
    assert L.info.value == E["lvl_info"]
    assert L.warn.value == E["lvl_warn"]
    assert L.err.value == E["lvl_err"]
    assert L.critical.value == E["lvl_critical"]
    assert L.off.value == E["lvl_off"]


def test_level_name_functions_differential():
    assert m.to_string_view(L.trace) == E["name_trace"]
    assert m.to_string_view(L.info) == E["name_info"]
    assert m.to_string_view(L.warn) == E["name_warn"]
    assert m.to_string_view(L.err) == E["name_err"]
    assert m.to_short_c_str(L.critical) == E["short_critical"]
    assert m.from_str("warning").value == E["from_str_warning"]
    assert m.from_str("trace").value == E["from_str_trace"]


def test_concrete_sink_differential():
    s = StdoutSink()
    assert s.level().value == E["sink_default_level"]
    assert s.should_log(L.info) == E["sink_should_info"]
    s.set_level(L.err)
    assert s.should_log(L.info) == E["sink_should_info_after"]


def test_null_sink_differential():
    # null_sink<null_mutex>, discovered by the derived_from_ match (both the
    # _mt and _st aliases name this one spec). A logger over it drives the
    # full pipeline with no observable output.
    ns = m.null_sinkNull_mutex()
    assert ns.level().value == E["null_default_level"]
    assert isinstance(ns, m.sink)
    lg = m.logger("nulllog", ns)
    lg.log(L.info, "swallowed")
    lg.flush()
    assert len(lg.sinks()) == E["null_n_sinks"]


def test_ansicolor_should_color_differential():
    # ansicolor sinks, discovered by the match; ctor takes the seeded
    # color_mode enum explicitly (the default VALUE is the P3096 gap).
    # always/never keeps should_color() tty-independent; the flattened
    # ansicolor_sink<M> surface (set_color_mode/should_color) drives it.
    a = m.ansicolor_stdout_sinkConsole_mutex(m.color_mode.always)
    assert a.should_color() == E["ansi_always"]
    a.set_color_mode(m.color_mode.never)
    assert a.should_color() == E["ansi_never"]


# --- Layer 3: invariants (the Tier-3 themes, structurally) ---

def test_sink_inheritance_chain():
    # stdout_sink<console_mutex> -> stdout_sink_base<console_mutex> -> sink, all
    # registered: the in-set abstract base is the real Python ancestor.
    s = StdoutSink()
    assert isinstance(s, m.sink)
    assert issubclass(StdoutSink, m.stdout_sink_baseConsole_mutex)
    assert issubclass(m.stdout_sink_baseConsole_mutex, m.sink)


def test_abstract_base_not_instantiable():
    # sink has pure virtuals; the binder must not have bound a constructor (BINDER-0011).
    with pytest.raises(TypeError):
        m.sink()


def test_shared_ptr_sink_roundtrip():
    # A bound concrete sink instance passes where shared_ptr<sink> is expected, and
    # sinks() hands back the shared_ptr as the bound sink type.
    s = StdoutSink()
    lg = m.logger("via_stdout", s)
    sinks = lg.sinks()
    assert len(sinks) == 1
    assert isinstance(sinks[0], m.sink)


def test_logger_surface_bound():
    for meth in ("log", "set_level", "level", "name", "should_log", "set_pattern",
                 "flush", "flush_on", "flush_level", "sinks", "clone",
                 "enable_backtrace", "disable_backtrace", "dump_backtrace"):
        assert hasattr(m.logger, meth), meth


def test_matched_sink_family():
    # Every sink the derived_from_ match discovers binds with sink as its
    # real Python ancestor; the stderr differential pins the second
    # stdout_sinks template behaviorally.
    for name in ("stdout_sinkConsole_mutex", "stdout_sinkConsole_nullmutex",
                 "stderr_sinkConsole_mutex", "stderr_sinkConsole_nullmutex",
                 "null_sinkNull_mutex",
                 "ansicolor_stdout_sinkConsole_mutex",
                 "ansicolor_stdout_sinkConsole_nullmutex",
                 "ansicolor_stderr_sinkConsole_mutex",
                 "ansicolor_stderr_sinkConsole_nullmutex"):
        cls = getattr(m, name, None)
        assert cls is not None, name
        assert issubclass(cls, m.sink), name
    # not_<named_<"ostream_sink*">> keeps the documented exclusion.
    assert not hasattr(m, "ostream_sinkConsole_mutex")
    assert not hasattr(m, "ostream_sinkConsole_nullmutex")
    es = m.stderr_sinkConsole_mutex()
    assert es.should_log(L.info) == E["stderr_should_info"]
