"""Differential test for the CLI11 binding (Layer-1 + Layer-3).

CLI::App is bound head-on via its string-collecting option API: add_option/add_flag return
an Option whose parsed values are kept as strings (results() == list[str]), and
parse(list[str]) drives a parse from a reversed forward command line (CLI11's
parse(vector<string>&) expects the reversed vector). oracle_native.cpp drives the EXACT
same option/flag specs and argv vectors through native CLI11 and emits every observable;
the assertions below compare the bound module against that ground truth, including the
two CLI::ParseError failure paths (RequiredError, ExtrasError) which nanobind surfaces as
Python exceptions.

NOTE: this run currently does NOT reach a clean pass. The binder gives raw-pointer-returning
methods (App.add_option/add_flag -> Option*, the fluent setters -> App*/Option*) the default
rv_policy::automatic, which for a pointer return means take_ownership; CLI11 owns each Option
through a std::unique_ptr in App, so Python's wrapper double-frees it. This crashes
(SIGABRT/SIGSEGV) during or after the first parse. See findings_draft/reflect-raw-ptr-return-
take-ownership-default.md for the minimized repro. The test is written against the CORRECT
expected behavior so it passes the moment the binder defaults pointer returns to a
non-owning policy.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("cli11_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def _rev(args):
    """CLI11's parse(vector<string>&) expects the reversed forward command line."""
    return list(reversed(args))


def test_basic_parse_differential():
    app = m.App("demo", "prog")
    file = app.add_option("--file,-f")
    verbose = app.add_flag("--verbose,-v")
    cnt = app.add_flag("--count,-c")
    app.parse(_rev(["--file", "x.txt", "-v", "-c", "-c"]))
    assert file.count() == E["file_count"]
    assert file.results()[0] == E["file_result0"]
    assert verbose.count() == E["verbose_count"]
    assert cnt.count() == E["cnt_count"]
    assert app.count("--file") == E["app_count_file"]
    assert file.get_single_name() == E["file_single_name"]
    assert len(file.get_snames()) == E["file_snames"]
    assert len(file.get_lnames()) == E["file_lnames"]
    assert app.get_description() == E["app_description"]


def test_multi_value_option_differential():
    app = m.App("multi", "prog")
    nums = app.add_option("--num,-n")
    nums.expected(3).take_all()
    app.parse(_rev(["--num", "1", "2", "3"]))
    assert nums.count() == E["num_count"]
    assert ",".join(nums.results()) == E["num_results"]


def test_required_missing_error_path_differential():
    app = m.App("req", "prog")
    app.add_option("--req").required(True)
    with pytest.raises(Exception) as ei:
        app.parse([])
    # CLI11's RequiredError -> nanobind RuntimeError; message must match the native oracle.
    assert E["req_error_kind"] == "RequiredError"
    assert E["req_error_what"] in str(ei.value)


def test_extras_error_path_differential():
    app = m.App("extras", "prog")
    app.add_flag("--known")
    with pytest.raises(Exception) as ei:
        app.parse(_rev(["--unknown"]))
    assert E["extras_error_kind"] == "ExtrasError"
    assert E["extras_error_what"] in str(ei.value)


# --- Layer 3: invariants (surface present, types) ---

def test_surface_bound():
    for meth in ("add_option", "add_flag", "parse", "count", "get_option",
                 "get_description", "get_name", "add_subcommand"):
        assert hasattr(m.App, meth), meth
    for meth in ("count", "empty", "results", "get_name", "required",
                 "expected", "take_all", "get_snames", "get_lnames"):
        assert hasattr(m.Option, meth), meth


def test_option_is_returned_type():
    app = m.App("t", "prog")
    o = app.add_option("--x")
    assert isinstance(o, m.Option)
