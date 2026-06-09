// A genuinely self-referential data structure: Node contains std::vector<Node>.
// This is the closest *constructible* analog to nlohmann::json's self-reference.
#pragma once
#include <vector>
namespace tree {
struct Node {
    int value;
    std::vector<Node> children;          // self-reference via a member
    Node() : value(0) {}
    explicit Node(int v) : value(v) {}
    void add(const Node& n) { children.push_back(n); }
    long count() const { return (long)children.size(); }
    int total() const { int s = value; for (auto& c : children) s += c.total(); return s; }
};
}
