// Tier-0 hand-written fixture (no external clone) to prove the corpus pipeline
// end-to-end with zero external variables. Exercises: data members, constructors,
// const/by-value methods, mutating methods, a static method, a free function, an enum.
#pragma once

namespace podfix {

struct Point {
    double x, y;

    Point() : x(0), y(0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}

    double norm2() const { return x * x + y * y; }   // const, by-value return
    void scale(double f) { x *= f; y *= f; }          // mutating

    static Point origin() { return Point(); }         // static factory
};

enum class Axis { X, Y, Z };

inline Point add(const Point& a, const Point& b) {    // free function
    return Point(a.x + b.x, a.y + b.y);
}

inline double dot(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y;
}

}  // namespace podfix
