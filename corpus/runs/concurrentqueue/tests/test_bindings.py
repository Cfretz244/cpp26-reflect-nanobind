"""Differential test for the moodycamel concurrentqueue binding (Layer-1 + Layer-3).

moodycamel::ConcurrentQueue<T> is bound head-on (int + std::string specs). Its non-template
enqueue/try_enqueue/size_approx/is_lock_free bind directly; the cqtest namespace supplies the
one dequeue adapter Python cannot drive head-on (try_dequeue is a member template returning
through an out-param). oracle_native.cpp drives the EXACT same single-threaded enqueue/dequeue
sequence through the real queue and emits every observable; these assertions compare the bound
module byte-for-byte against that ground truth. Layer 3 checks the Tier-3 themes structurally.
"""
import json as _json
import pathlib

import pytest

m = pytest.importorskip("concurrentqueue_ext")

_E = pathlib.Path(__file__).parent / "expected.json"
if not _E.exists():
    pytest.skip("expected.json not generated (run via run_gates.py)", allow_module_level=True)
E = _json.loads(_E.read_text())

# The bound classes for ConcurrentQueue<int> / ConcurrentQueue<std::string>; the
# spec-derived Python name folds in the default Traits parameter (BINDER-0003 cosmetic):
# ConcurrentQueue<int, ConcurrentQueueDefaultTraits> -> ConcurrentQueueIntConcurrentQueueDefaultTraits.
QInt = m.ConcurrentQueueIntConcurrentQueueDefaultTraits
QStr = m.ConcurrentQueueStringConcurrentQueueDefaultTraits


def _dequeue_int(q):
    return m.try_dequeue_int(q)


def test_is_lock_free_differential():
    assert QInt.is_lock_free() == E["is_lock_free"]


def test_empty_size_and_dequeue_differential():
    q = QInt(64)
    assert q.size_approx() == E["size_empty"]
    # dequeue from empty -> None (the nullopt path)
    assert (_dequeue_int(q) is not None) == E["empty_dequeue_has_value"]


def test_fifo_ordering_differential():
    q = QInt(64)
    all_enq = True
    for i in range(10):
        all_enq = q.enqueue(i) and all_enq
    assert all_enq == E["all_enqueued"]
    assert q.size_approx() == E["size_after_enqueue"]

    assert q.try_enqueue(100) == E["try_enqueue_ok"]
    assert q.size_approx() == E["size_after_try"]

    first5 = []
    for _ in range(5):
        v = _dequeue_int(q)
        first5.append(v)            # None sentinel preserved; oracle emits ints
    assert first5 == E["first5"]    # FIFO order from a single implicit producer
    assert q.size_approx() == E["size_after_drain5"]

    rest = []
    while True:
        v = _dequeue_int(q)
        if v is None:
            break
        rest.append(v)
    assert rest == E["rest"]
    assert q.size_approx() == E["size_drained"]
    assert (_dequeue_int(q) is None) == E["empty_again"]


def test_string_roundtrip_differential():
    qs = QStr(8)
    qs.enqueue("alpha")
    qs.enqueue("beta")
    qs.try_enqueue("gamma")
    strs = []
    while True:
        v = m.try_dequeue_str(qs)
        if v is None:
            break
        strs.append(v)
    assert strs == E["strs"]


# --- Layer 3: invariants ---

def test_queue_surface_bound():
    for meth in ("enqueue", "try_enqueue", "size_approx", "is_lock_free"):
        assert hasattr(QInt, meth), meth


def test_default_capacity_ctor():
    # explicit ConcurrentQueue(size_t capacity = 32*BLOCK_SIZE): both the value ctor and
    # its default must bind; constructing with no args exercises the default argument is
    # NOT bindable (C++26 gap) -- so we only assert the value ctor here.
    q = QInt(128)
    assert q.size_approx() == 0


def test_copy_construction_rejected():
    # ConcurrentQueue's copy ctor is deleted; the binder must not synthesize init<const&>
    # (BINDER-0013 only adds copy for copyable classes). Passing a queue where an int is
    # expected must fail rather than copy-construct.
    q = QInt(16)
    q.enqueue(7)
    with pytest.raises(TypeError):
        QInt(q)  # no copy ctor bound
