"""Differential test for the SQLiteCpp 3.3.3 binding (Layer-1 + Layer-3).

The binder binds SQLite::Database / Statement / Column / Transaction / Exception head-on.
oracle_native.cpp drives the SAME scenario natively against an in-memory database and emits
its results; this test drives the identical scenario through the bound module and asserts
equality on real behavior (typed column values, names, declared types, change counts,
transaction visibility, and the constraint-violation error path), plus Layer-3 invariants.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("sqlitecpp_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# Open-mode flag ints surfaced through the sqlfix fixture (SQLite::OPEN_* namespace consts).
OPEN_READWRITE = m.open_readwrite()
OPEN_CREATE = m.open_create()


def make_db():
    """Open an in-memory database with READWRITE|CREATE, matching the oracle.

    Default-argument VALUES are a C++26 reflection gap (not bound), so every defaulted
    parameter of the Database ctor (busy-timeout, vfs) is passed explicitly with the same
    values the native oracle's C++ defaults supply (0 ms, default vfs ""). Both sides thus
    drive an identical call.
    """
    return m.Database(":memory:", OPEN_READWRITE | OPEN_CREATE, 0, "unix")


def populated_db():
    """Reproduce the oracle's exact scenario: create table + insert three rows."""
    db = make_db()
    db.exec("CREATE TABLE widgets (id INTEGER PRIMARY KEY, name TEXT UNIQUE, qty INTEGER, weight REAL)")
    ins = m.Statement(db, "INSERT INTO widgets (id, name, qty, weight) VALUES (?, ?, ?, ?)")
    rows = [(1, "alpha", 10, 1.5), (2, "beta", 20, 2.25), (3, "gamma", 30, 3.125)]
    for (rid, name, qty, weight) in rows:
        ins.reset()
        ins.bind(1, rid)
        ins.bind(2, name)
        ins.bind(3, qty)
        ins.bind(4, weight)
        ins.exec()
    return db


# --- Layer 1: differential vs the native oracle ---

def test_open_and_table_existence():
    db = make_db()
    assert db.getFilename() == E["filename"]
    assert db.tableExists("widgets") == E["exists_before"]
    db.exec("CREATE TABLE widgets (id INTEGER PRIMARY KEY, name TEXT UNIQUE, qty INTEGER, weight REAL)")
    assert db.tableExists("widgets") == E["exists_after"]


def test_insert_change_counts():
    db = make_db()
    db.exec("CREATE TABLE widgets (id INTEGER PRIMARY KEY, name TEXT UNIQUE, qty INTEGER, weight REAL)")
    ins = m.Statement(db, "INSERT INTO widgets (id, name, qty, weight) VALUES (?, ?, ?, ?)")
    total = 0
    for (rid, name, qty, weight) in [(1, "alpha", 10, 1.5), (2, "beta", 20, 2.25), (3, "gamma", 30, 3.125)]:
        ins.reset()
        ins.bind(1, rid)
        ins.bind(2, name)
        ins.bind(3, qty)
        ins.bind(4, weight)
        total += ins.exec()
    assert total == E["insert_total_changes"]
    assert db.getLastInsertRowid() == E["last_rowid"]
    assert db.getTotalChanges() == E["total_changes"]


def test_select_typed_columns_and_metadata():
    db = populated_db()
    q = m.Statement(db, "SELECT id, name, qty, weight FROM widgets ORDER BY id")
    assert q.getColumnCount() == E["col_count"]
    assert q.getColumnName(0) == E["col_name_0"]
    assert q.getColumnName(1) == E["col_name_1"]
    assert q.getColumnIndex("weight") == E["col_idx_weight"]

    got = []
    decl_id = decl_name = None
    type_code_name = type_code_qty = None
    while q.executeStep():
        c_id = q.getColumn(0)
        c_name = q.getColumn(1)
        c_qty = q.getColumn(2)
        c_weight = q.getColumn(3)
        decl_id = q.getColumnDeclaredType(0)
        decl_name = q.getColumnDeclaredType(1)
        type_code_name = c_name.getType()
        type_code_qty = c_qty.getType()
        got.append({
            "id": c_id.getInt(),
            "name": c_name.getString(),
            "name_getText": c_name.getText(""),  # default apDefaultValue="" passed explicitly (C++26 gap)
            "colname": c_name.getName(),
            "qty": c_qty.getInt64(),
            "weight": c_weight.getDouble(),
            "id_isInteger": c_id.isInteger(),
            "name_isText": c_name.isText(),
            "weight_isFloat": c_weight.isFloat(),
        })
    assert got == E["rows"]
    assert decl_id == E["decl_type_id"]
    assert decl_name == E["decl_type_name"]
    assert type_code_name == E["type_code_name"]
    assert type_code_qty == E["type_code_qty"]
    # The type codes must equal the SQLite::TEXT / SQLite::INTEGER constants the fixture surfaces.
    assert type_code_name == m.type_text()
    assert type_code_qty == m.type_integer()


def test_transaction_rollback_then_commit_visibility():
    db = populated_db()

    tx = m.Transaction(db)
    db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (99, 'ghost', 0, 0.0)")
    tx.rollback()
    c = m.Statement(db, "SELECT COUNT(*) FROM widgets WHERE id = 99")
    c.executeStep()
    assert c.getColumn(0).getInt() == E["ghost_after_rollback"]

    tx2 = m.Transaction(db)
    db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (100, 'committed', 0, 0.0)")
    tx2.commit()
    c2 = m.Statement(db, "SELECT COUNT(*) FROM widgets WHERE id = 100")
    c2.executeStep()
    assert c2.getColumn(0).getInt() == E["committed_after_commit"]

    cf = m.Statement(db, "SELECT COUNT(*) FROM widgets")
    cf.executeStep()
    assert cf.getColumn(0).getInt() == E["final_count"]


def test_constraint_violation_error_path():
    db = populated_db()
    # Duplicate UNIQUE name -> SQLite::Exception (derives std::runtime_error -> Python exception).
    with pytest.raises(Exception) as ei:
        db.exec("INSERT INTO widgets (id, name, qty, weight) VALUES (4, 'alpha', 0, 0.0)")
    assert E["constraint_threw"] is True
    assert E["constraint_errcode"] == 19  # SQLITE_CONSTRAINT
    assert E["constraint_msg_has_unique"] is True
    # The real SQLite message ("UNIQUE constraint failed: ...") must reach Python.
    assert "UNIQUE" in str(ei.value)


def test_transaction_behavior_enum():
    # Construct a transaction with an explicit behavior (the real scoped enum).
    db = populated_db()
    tx = m.Transaction(db, m.TransactionBehavior.IMMEDIATE)
    db.exec("UPDATE widgets SET qty = qty + 1")
    tx.commit()
    q = m.Statement(db, "SELECT qty FROM widgets WHERE id = 1")
    q.executeStep()
    assert q.getColumn(0).getInt() == 11  # 10 + 1


# --- Layer 3: invariants ---

def test_surface_bound():
    for meth in ("exec", "tableExists", "execAndGet", "getLastInsertRowid",
                 "getChanges", "getTotalChanges", "getFilename"):
        assert hasattr(m.Database, meth), meth
    for meth in ("bind", "executeStep", "getColumn", "getColumnName",
                 "getColumnCount", "getColumnIndex", "reset"):
        assert hasattr(m.Statement, meth), meth
    for meth in ("getInt", "getInt64", "getDouble", "getText", "getString",
                 "getName", "getType", "isNull", "isText", "isInteger", "isFloat"):
        assert hasattr(m.Column, meth), meth
    for meth in ("commit", "rollback"):
        assert hasattr(m.Transaction, meth), meth


def test_transaction_behavior_values():
    # The scoped enum is bound with all three behaviors.
    assert set(("DEFERRED", "IMMEDIATE", "EXCLUSIVE")).issubset(set(dir(m.TransactionBehavior)))
