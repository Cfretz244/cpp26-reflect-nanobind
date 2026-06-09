// Tier-4 hand-written fixture proving the two-stage codegen path: a Python subclass must be
// able to override a C++ virtual and have C++ dispatch into it (requires a generated trampoline).
#pragma once
#include <string>

namespace shapes {

struct Shape {
    Shape() = default;
    virtual ~Shape() = default;
    virtual double area() const = 0;                    // pure virtual
    virtual std::string name() const { return "shape"; } // non-pure virtual
};

// Concrete subclass — gives the native oracle a differential anchor (no Python involved).
struct Circle : Shape {
    double r;
    Circle() : r(0) {}
    explicit Circle(double r_) : r(r_) {}
    double area() const override { return 3.141592653589793 * r * r; }
    std::string name() const override { return "circle"; }
};

// C++ drivers that call the virtuals through a base reference, so a test can demonstrate
// that a Python override is dispatched into from C++.
inline double call_area(const Shape& s) { return s.area(); }
inline std::string call_name(const Shape& s) { return s.name(); }

}  // namespace shapes
