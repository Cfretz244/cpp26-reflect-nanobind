"""Differential + invariant tests for the taskflow v4.0.0 binding.

Layer 1 (differential): the native oracle (oracle_native.cpp) builds the SAME fixed
diamond graph through the SAME tftest fixture and emits expected.json; here we drive the
identical scenario through the bound module and assert equality on every graph-shape
number, the executor worker count, the post-run task-body count, and the dependency
order CONSTRAINTS (not exact interleavings -- those are non-deterministic).

Layer 3 (invariants): the real classes are present with their head-on surface; Task
identity dunders behave; dump() mentions every task; the diamond's predecessor/successor
shape is internally consistent.
"""
import json
import pathlib

import pytest

import taskflow_ext as m

HERE = pathlib.Path(__file__).parent
EXPECTED = json.loads((HERE / "expected.json").read_text())


def build_diamond():
    return m.Diamond()


# ---------- Layer 1: differential vs the native oracle ----------

def test_taskflow_shape_matches_oracle():
    d = build_diamond()
    tf = d.taskflow()
    assert tf.name() == EXPECTED["tf_name"]
    assert tf.num_tasks() == EXPECTED["num_tasks"]
    assert tf.empty() == EXPECTED["tf_empty"]


def test_per_task_shape_matches_oracle():
    d = build_diamond()
    tasks = {"A": d.task_a(), "B": d.task_b(), "C": d.task_c(), "D": d.task_d()}
    for k, t in tasks.items():
        p = f"task_{k}_"
        assert t.name() == EXPECTED[p + "name"], k
        assert t.num_successors() == EXPECTED[p + "num_successors"], k
        assert t.num_predecessors() == EXPECTED[p + "num_predecessors"], k
        assert t.num_strong_dependencies() == EXPECTED[p + "num_strong"], k
        assert t.num_weak_dependencies() == EXPECTED[p + "num_weak"], k
        assert t.empty() == EXPECTED[p + "empty"], k
        assert t.has_work() == EXPECTED[p + "has_work"], k


def test_task_identity_dunders_match_oracle():
    d = build_diamond()
    A, B = d.task_a(), d.task_b()
    assert (A == A) == EXPECTED["A_eq_A"]
    assert (A == B) == EXPECTED["A_eq_B"]
    assert (A != B) == EXPECTED["A_ne_B"]


def test_executor_worker_count_matches_oracle():
    eng = m.Engine(3)
    assert eng.executor().num_workers() == EXPECTED["num_workers"]


def test_run_executes_all_tasks_matches_oracle():
    d = build_diamond()
    eng = m.Engine(3)
    ran = eng.run(d)
    assert ran == EXPECTED["bodies_ran"]
    assert d.counter() == EXPECTED["counter_after"]
    assert ran == d.taskflow().num_tasks()


def test_execution_order_constraints_match_oracle():
    d = build_diamond()
    eng = m.Engine(2)
    eng.run(d)
    order = list(d.order())
    assert len(order) == EXPECTED["order_len"]
    ix = {n: order.index(n) for n in ("A", "B", "C", "D")}
    # The oracle recorded each constraint as a boolean over a real run; assert the
    # SAME constraints hold here (dependency edges, not exact interleaving).
    assert (ix["A"] == 0) == EXPECTED["A_first"]
    assert (ix["D"] == len(order) - 1) == EXPECTED["D_last"]
    assert (ix["A"] < ix["B"]) == EXPECTED["A_before_B"]
    assert (ix["A"] < ix["C"]) == EXPECTED["A_before_C"]
    assert (ix["B"] < ix["D"]) == EXPECTED["B_before_D"]
    assert (ix["C"] < ix["D"]) == EXPECTED["C_before_D"]
    # And the diamond constraints are genuinely satisfied (the oracle's booleans
    # are all True for a correct run -- pin that here too).
    assert ix["A"] == 0
    assert ix["D"] == 3
    assert ix["A"] < ix["B"] < ix["D"]
    assert ix["A"] < ix["C"] < ix["D"]


# ---------- Layer 3: invariants ----------

def test_core_classes_present():
    for name in ("Taskflow", "Task", "Executor", "Diamond", "Engine"):
        assert hasattr(m, name), name


def test_taskflow_head_on_surface():
    tf = m.Taskflow()
    assert tf.num_tasks() == 0
    assert tf.empty() is True
    for meth in ("name", "num_tasks", "empty", "dump", "clear"):
        assert hasattr(tf, meth), meth


def test_task_head_on_surface():
    d = build_diamond()
    A = d.task_a()
    for meth in ("name", "num_successors", "num_predecessors",
                 "num_strong_dependencies", "num_weak_dependencies",
                 "empty", "has_work", "hash_value"):
        assert hasattr(A, meth), meth
    assert isinstance(A.hash_value(), int)


def test_executor_head_on_surface():
    ex = m.Engine(2).executor()
    for meth in ("num_workers", "num_topologies", "num_taskflows", "num_observers"):
        assert hasattr(ex, meth), meth
    assert ex.num_topologies() == 0
    assert ex.num_taskflows() == 0


def test_dump_mentions_every_task():
    d = build_diamond()
    dot = d.taskflow().dump()
    assert isinstance(dot, str) and dot
    for name in ("A", "B", "C", "D"):
        assert name in dot, name


def test_diamond_shape_is_self_consistent():
    # Sum of successors == sum of predecessors == number of edges (4 in the diamond).
    d = build_diamond()
    tasks = [d.task_a(), d.task_b(), d.task_c(), d.task_d()]
    succ = sum(t.num_successors() for t in tasks)
    pred = sum(t.num_predecessors() for t in tasks)
    assert succ == pred == 4


def test_empty_default_task_handle():
    # A default-constructed (empty) Task handle reports empty / no work.
    t = m.Task()
    assert t.empty() is True
    assert t.has_work() is False
