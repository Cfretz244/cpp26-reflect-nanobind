"""Differential test for the Abseil CRC32C binding (Layer-1 differential + Layer-3).

absl::crc32c_t binds directly: explicit-uint32 constructor, explicit operator uint32_t
-> __int__ (the value channel), namespace operator<< -> __str__ ("%08x" hex). The free
computation/arithmetic functions are exposed via the crctest fixture namespace
(compute/extend/extend_by_zeroes/concat/remove_prefix/remove_suffix/eq/value).
oracle_native.cpp drives the identical surface natively; any divergence is the
binding layer's.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("abseil_crc_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())


def test_compute_differential():
    c = m.compute("hello world")
    assert int(c) == E["crc_hello_world"]          # __int__ == native uint32 value
    assert m.value(c) == E["crc_hello_world"]      # fixture channel agrees
    assert str(c) == E["crc_hello_world_str"]      # __str__ == native "%08x" stream form
    assert int(m.compute("")) == E["crc_empty"]


def test_extend_differential():
    ext = m.extend(m.compute("hello "), "world")
    assert int(ext) == E["crc_extended"]
    assert m.eq(ext, m.compute("hello world")) == E["extend_matches_full"]
    assert int(m.extend_by_zeroes(m.compute("hello "), 5)) == E["crc_zeroes"]


def test_concat_differential():
    cat = m.concat(m.compute("hello "), m.compute("world"), len("world"))
    assert int(cat) == E["crc_concat"]
    assert m.eq(cat, m.compute("hello world")) == E["concat_matches_full"]


def test_remove_prefix_suffix_differential():
    full = m.compute("hello world")
    rp = m.remove_prefix(m.compute("hello "), full, len("world"))
    assert int(rp) == E["crc_rm_prefix"]
    assert m.eq(rp, m.compute("world")) == E["rm_prefix_matches"]
    rs = m.remove_suffix(full, m.compute("world"), len("world"))
    assert int(rs) == E["crc_rm_suffix"]
    assert m.eq(rs, m.compute("hello ")) == E["rm_suffix_matches"]


# --- Layer 3: structural invariants ---

def test_value_type_surface():
    # Explicit uint32 constructor round-trips through __int__.
    assert int(m.crc32c_t(0xDEADBEEF)) == 0xDEADBEEF
    assert int(m.crc32c_t(0)) == 0
    # eq() is reflexive and detects inequality.
    assert m.eq(m.crc32c_t(7), m.crc32c_t(7))
    assert not m.eq(m.crc32c_t(7), m.crc32c_t(8))
    # __str__ is the 8-hex-digit stream form.
    assert str(m.crc32c_t(0xDEADBEEF)) == "deadbeef"
