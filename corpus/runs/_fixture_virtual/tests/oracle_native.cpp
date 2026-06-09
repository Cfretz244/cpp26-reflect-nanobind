// Native oracle: ground-truth for the concrete Circle path (no Python involved).
#include "shapes.h"
#include <cstdio>

int main() {
    shapes::Circle c(2.0);
    std::printf(
        "{ \"circle2_area\": %.17g, \"call_area_circle2\": %.17g }\n",
        c.area(), shapes::call_area(c));
    return 0;
}
