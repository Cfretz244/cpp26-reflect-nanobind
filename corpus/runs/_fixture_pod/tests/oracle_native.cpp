// Native C++ oracle: exercises the SAME API as the bindings and prints ground-truth
// JSON to stdout. The Python test asserts the bindings reproduce these exact values.
#include "pod_fixture.h"
#include <cstdio>

int main() {
    using namespace podfix;

    Point p(3.0, 4.0);
    Point s(3.0, 4.0); s.scale(2.0);
    Point sum = add(Point(1, 2), Point(3, 4));
    Point o = Point::origin();

    std::printf(
        "{\n"
        "  \"norm2_3_4\": %.17g,\n"
        "  \"add_x\": %.17g, \"add_y\": %.17g,\n"
        "  \"dot_12_34\": %.17g,\n"
        "  \"origin_x\": %.17g, \"origin_y\": %.17g,\n"
        "  \"scale2_x\": %.17g, \"scale2_y\": %.17g,\n"
        "  \"axis_x\": %d, \"axis_y\": %d, \"axis_z\": %d\n"
        "}\n",
        p.norm2(),
        sum.x, sum.y,
        dot(Point(1, 2), Point(3, 4)),
        o.x, o.y,
        s.x, s.y,
        (int)Axis::X, (int)Axis::Y, (int)Axis::Z);
    return 0;
}
