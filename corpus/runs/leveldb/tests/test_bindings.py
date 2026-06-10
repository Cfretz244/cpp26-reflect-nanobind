"""Differential test for the leveldb 1.23 binding (Layer-1 + Layer-3).

The binder binds leveldb's real Status/Slice/Options/ReadOptions/WriteOptions/WriteBatch/
CompressionType head-on. leveldb::DB cannot be bound head-on (LevelDB is built -fno-rtti, so
the archive lacks DB's std::type_info and nanobind's RTTI-based registry fails to link; see
findings_draft/leveldb-fno-rtti-polymorphic-typeinfo-missing.md), so the DB is driven through
leveldbtest.KVStore -- a non-polymorphic handle whose methods forward verbatim to the genuine
DB::Put/Get/Delete/Write and bridge the DB::Open (DB**) and DB::Get (std::string*) out-params.

oracle_native.cpp drives the SAME open-put-get-delete-get / WriteBatch / re-open scenario
natively against the real DB and emits its results; the assertions compare the bound module
against that ground truth (shared compiler + shared leveldb). Each scenario opens a fresh DB
under tests/build/ so the differential is deterministic. The Status / Slice / WriteBatch
objects exchanged with KVStore are the real bound classes.
"""
import json as _json
import pathlib
import shutil

import pytest

m = pytest.importorskip("leveldb_ext")

_HERE = pathlib.Path(__file__).parent
_E = _HERE / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

_DBROOT = _HERE / "build" / "pydb"


def _fresh_path(name):
    p = _DBROOT / name
    if p.exists():
        shutil.rmtree(p, ignore_errors=True)
    p.parent.mkdir(parents=True, exist_ok=True)
    return str(p)


def _open(path, create=True):
    store = m.KVStore()
    opts = m.Options()
    opts.create_if_missing = create
    st = store.open(opts, path)
    return store, st


# --- Layer 1: differential vs the native oracle ---

def test_open_put_get_delete_get():
    path = _fresh_path("main")
    store, st = _open(path)
    assert st.ok() is True
    assert st.ok() == E["open_ok"]
    assert st.ToString() == E["open_str"]
    assert store.is_open() is True

    wopts = m.WriteOptions()
    ropts = m.ReadOptions()

    assert store.put(wopts, "alpha", "one").ok() == E["put_alpha_ok"]
    assert store.put(wopts, "beta", "two").ok() == E["put_beta_ok"]

    g = store.get(ropts, "alpha")
    assert g.status.ok() == E["get_alpha_ok"]
    assert g.value == E["get_alpha_val"]
    assert g.found is True

    # missing key -> NotFound
    gm = store.get(ropts, "missing")
    assert gm.status.ok() == E["get_missing_ok"]
    assert gm.status.IsNotFound() == E["get_missing_notfound"]
    assert gm.status.ToString() == E["get_missing_str"]
    assert gm.found is False

    # delete alpha, get alpha -> NotFound
    assert store.delete_key(wopts, "alpha").ok() == E["del_alpha_ok"]
    gd = store.get(ropts, "alpha")
    assert gd.status.IsNotFound() == E["get_deleted_notfound"]


def test_write_batch_atomicity():
    path = _fresh_path("batch")
    store, st = _open(path)
    assert st.ok()
    wopts = m.WriteOptions()
    ropts = m.ReadOptions()

    store.put(wopts, "alpha", "one")
    store.put(wopts, "beta", "two")

    batch = m.WriteBatch()                       # WriteBatch bound head-on
    m.batch_put(batch, "gamma", "three")         # real WriteBatch::Put (via std::string front)
    m.batch_delete(batch, "beta")                # real WriteBatch::Delete
    assert batch.ApproximateSize() == E["batch_approx_size"]   # head-on WriteBatch method
    assert store.write(wopts, batch).ok() == E["write_batch_ok"]

    gg = store.get(ropts, "gamma")
    assert gg.status.ok() == E["get_gamma_ok"]
    assert gg.value == E["get_gamma_val"]
    gb = store.get(ropts, "beta")
    assert gb.status.IsNotFound() == E["get_beta_after_batch_notfound"]


def test_reopen_persistence():
    path = _fresh_path("persist")
    store, st = _open(path)
    assert st.ok()
    wopts = m.WriteOptions()
    batch = m.WriteBatch()
    m.batch_put(batch, "gamma", "three")
    store.write(wopts, batch)
    store.close()  # close the underlying DB

    store2, st2 = _open(path, create=False)
    assert st2.ok() == E["reopen_ok"]
    g = store2.get(m.ReadOptions(), "gamma")
    assert g.status.ok() == E["reopen_get_gamma_ok"]
    assert g.value == E["reopen_get_gamma_val"]


def test_slice_value_surface():
    # By-value Slice state is safe to read directly off a Python-constructed Slice (size_ is
    # stored by value). The pointer-dereferencing surface (ToString/compare/starts_with) is
    # exercised on a LIVE Slice via slice_info below -- a Python-constructed Slice's data_
    # dangles (its backing temporary is freed after __init__); see
    # findings_draft/nonowning-view-ctor-from-string-dangles.md.
    sl = m.Slice("hello")
    assert sl.size() == E["slice_size"]
    assert sl.empty() is False
    assert m.Slice("").empty() is True
    assert m.Slice("").size() == 0


def test_slice_deref_surface_via_live_slice():
    info = m.slice_info("hello", "he")            # drives the REAL Slice methods on a live Slice
    assert info.size == E["slice_size"]
    assert info.str == E["slice_str"]
    assert info.starts_with_prefix == E["slice_starts_with"]
    assert info.compare_self == E["slice_compare"]
    assert info.eq_self == E["slice_eq"]
    assert info.empty is False


def test_compression_enum():
    assert m.CompressionType.kNoCompression.value == E["comp_none"]
    assert m.CompressionType.kSnappyCompression.value == E["comp_snappy"]
    # the enum is also usable as an Options field value
    o = m.Options()
    o.compression = m.CompressionType.kNoCompression
    assert o.compression == m.CompressionType.kNoCompression


# --- Layer 3: invariants ---

def test_surface_present():
    for meth in ("ok", "IsNotFound", "IsCorruption", "ToString"):
        assert hasattr(m.Status, meth), meth
    for meth in ("data", "size", "empty", "ToString", "compare", "starts_with"):
        assert hasattr(m.Slice, meth), meth
    for meth in ("Put", "Delete", "Clear", "Append", "ApproximateSize"):
        assert hasattr(m.WriteBatch, meth), meth


def test_options_fields():
    o = m.Options()
    o.create_if_missing = True
    o.error_if_exists = False
    assert o.create_if_missing is True
    r = m.ReadOptions()
    r.verify_checksums = True
    assert r.verify_checksums is True
    w = m.WriteOptions()
    w.sync = True
    assert w.sync is True
