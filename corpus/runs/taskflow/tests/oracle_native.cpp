// Native C++ ground-truth oracle for the taskflow binding (Layer-1 differential).
// Builds the EXACT same fixed diamond graph the Python test drives through the bound
// module (via the shared tftest.Diamond / tftest::Engine fixture), then emits every
// graph-shape observable as one JSON object. Shared compiler + shared header-only
// taskflow => any divergence is the binding layer's.
//
// The fixture header (tftest.h) is the single source of truth for the graph: both this
// oracle and the module build it through tftest::Diamond, so the two sides cannot drift.
// Execution ORDER is non-deterministic (worker interleaving), so it is NOT emitted here;
// the differential checks order CONSTRAINTS, which the oracle records as booleans over a
// real run, and the Python side checks the same constraints over its own run.
#include "../binding/tftest.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

static std::string esc(const std::string& s) {
  std::string o;
  for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
  return o;
}

int main() {
  std::vector<std::pair<std::string, std::string>> kv;
  auto add_i = [&](const std::string& k, long long v) {
    kv.emplace_back(k, std::to_string(v));
  };
  auto add_b = [&](const std::string& k, bool v) {
    kv.emplace_back(k, v ? "true" : "false");
  };
  auto add_s = [&](const std::string& k, const std::string& v) {
    kv.emplace_back(k, "\"" + esc(v) + "\"");
  };

  tftest::Diamond d;
  tf::Taskflow* tf = d.taskflow();

  // Taskflow-level shape.
  add_s("tf_name", tf->name());
  add_i("num_tasks", static_cast<long long>(tf->num_tasks()));
  add_b("tf_empty", tf->empty());

  // Per-task shape (the diamond: A->{B,C}->D).
  struct TInfo { const char* k; tf::Task t; };
  TInfo infos[] = {
    {"A", d.task_a()}, {"B", d.task_b()}, {"C", d.task_c()}, {"D", d.task_d()},
  };
  for (auto& ti : infos) {
    std::string p = std::string("task_") + ti.k + "_";
    add_s(p + "name", ti.t.name());
    add_i(p + "num_successors", static_cast<long long>(ti.t.num_successors()));
    add_i(p + "num_predecessors", static_cast<long long>(ti.t.num_predecessors()));
    add_i(p + "num_strong", static_cast<long long>(ti.t.num_strong_dependencies()));
    add_i(p + "num_weak", static_cast<long long>(ti.t.num_weak_dependencies()));
    add_b(p + "empty", ti.t.empty());
    add_b(p + "has_work", ti.t.has_work());
  }

  // Task identity dunders.
  add_b("A_eq_A", d.task_a() == d.task_a());
  add_b("A_eq_B", d.task_a() == d.task_b());
  add_b("A_ne_B", d.task_a() != d.task_b());

  // Executor worker count from an EXPLICIT ctor arg.
  tftest::Engine eng(3);
  add_i("num_workers", static_cast<long long>(eng.executor()->num_workers()));

  // A real run: bodies-run count, and order-constraint booleans over the order log.
  std::size_t ran = eng.run(d);
  add_i("bodies_ran", static_cast<long long>(ran));
  add_i("counter_after", static_cast<long long>(d.counter()));

  std::vector<std::string> order = d.order();
  auto idx = [&](const std::string& n) -> long long {
    for (std::size_t i = 0; i < order.size(); ++i) if (order[i] == n) return (long long)i;
    return -1;
  };
  long long ia = idx("A"), ib = idx("B"), ic = idx("C"), id = idx("D");
  add_i("order_len", static_cast<long long>(order.size()));
  add_b("A_first", ia == 0);
  add_b("D_last", id == (long long)order.size() - 1);
  add_b("A_before_B", ia < ib);
  add_b("A_before_C", ia < ic);
  add_b("B_before_D", ib < id);
  add_b("C_before_D", ic < id);

  std::cout << "{";
  for (std::size_t i = 0; i < kv.size(); ++i) {
    if (i) std::cout << ",";
    std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
  }
  std::cout << "}" << std::endl;
  return 0;
}
