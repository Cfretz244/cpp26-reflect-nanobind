// tftest -- fixture for the taskflow run, shared by binding.cpp and the native oracle.
//
// Taskflow's task-construction front (FlowBuilder::emplace / Task::precede / succeed)
// is entirely templated over C++ callables and variadic Task packs -- none of it is
// expressible from Python head-on. This fixture builds a SMALL FIXED diamond task
// graph in C++ (A -> {B, C} -> D) whose tasks append their names to a shared
// execution log and bump a shared atomic counter, so Python can OBSERVE both the
// graph SHAPE (via the real tf::Taskflow / tf::Task handles it hands out, queried
// head-on) and the EXECUTION (via the order log + counter after a real
// tf::Executor::run drives it). It binds NO behavior itself beyond wiring + run:
// every number the differential checks comes from a real taskflow class method.
//
// <bit> first: taskflow v4.0.0 uses std::bit_ceil/std::bit_width (wsq.hpp,
// executor.hpp) but never includes <bit>; on this from-source libc++ that header is
// not pulled in transitively, so the umbrella header fails to compile without it.
// (Library-side missing-include finding; see findings_draft.)
#pragma once

#include <bit>
#include <taskflow/taskflow.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace tftest {

// A fixed diamond graph plus the observability the real API cannot give Python.
// Owns the Taskflow (so the Task handles into it stay valid) and the log/counter
// the tasks write. Built once; run via run(executor) which blocks until done.
class Diamond {
public:
  Diamond() : tf_("diamond") {
    // A -> B, A -> C, B -> D, C -> D : a 4-task diamond.
    a_ = tf_.emplace([this]{ record("A"); }).name("A");
    b_ = tf_.emplace([this]{ record("B"); }).name("B");
    c_ = tf_.emplace([this]{ record("C"); }).name("C");
    d_ = tf_.emplace([this]{ record("D"); }).name("D");
    a_.precede(b_, c_);
    b_.precede(d_);
    c_.precede(d_);
  }

  // The real Taskflow + Task handles, handed out for head-on querying.
  // taskflow() returns a POINTER (not tf::Taskflow&): a method returning a
  // reference to a registered class currently defaults to rv_policy::copy, which
  // aborts on the move-only tf::Taskflow (see findings_draft). A raw class-pointer
  // return is borrowed correctly (reference_internal). The pointee outlives the
  // returned wrapper (this Diamond owns it), so reference_internal is sound.
  tf::Taskflow* taskflow() { return &tf_; }
  tf::Task task_a() const { return a_; }
  tf::Task task_b() const { return b_; }
  tf::Task task_c() const { return c_; }
  tf::Task task_d() const { return d_; }

  // Drive the graph through a REAL executor (pointer, not ref -- same rv_policy
  // reason) and block until complete. Returns the number of task bodies that ran
  // (== num_tasks for this acyclic graph).
  std::size_t run(tf::Executor* ex) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      order_.clear();
    }
    counter_.store(0, std::memory_order_relaxed);
    ex->run(tf_).wait();
    return counter_.load(std::memory_order_relaxed);
  }

  // The execution-order log (task names in the order their bodies started), for
  // asserting dependency ORDER CONSTRAINTS (not exact interleavings).
  std::vector<std::string> order() const {
    std::lock_guard<std::mutex> lk(mu_);
    return order_;
  }
  std::size_t counter() const { return counter_.load(std::memory_order_relaxed); }

private:
  void record(const char* n) {
    counter_.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(mu_);
    order_.emplace_back(n);
  }

  tf::Taskflow tf_;
  tf::Task a_, b_, c_, d_;
  std::atomic<std::size_t> counter_{0};
  mutable std::mutex mu_;
  std::vector<std::string> order_;
};

// Owns a tf::Executor built with an EXPLICIT worker count. tf::Executor's only
// constructor is Executor(size_t N, unique_ptr<WorkerInterface> wif = nullptr) --
// the move-only/opaque 2nd parameter means the binder binds no __init__ for it, so
// Python cannot construct one directly. This fixture constructs it with N workers
// (the worker-count theme: num_workers() is checked against the N passed here) and
// hands out the REAL tf::Executor for head-on querying.
class Engine {
public:
  explicit Engine(std::size_t n) : ex_(n) {}

  // The real Executor, by pointer (rv_policy: reference_internal -- pointee
  // outlives the wrapper since this Engine owns it).
  tf::Executor* executor() { return &ex_; }

  // Convenience: run a Diamond's graph on this executor; returns bodies-run count.
  std::size_t run(Diamond& d) { return d.run(&ex_); }

private:
  tf::Executor ex_;
};

}  // namespace tftest
