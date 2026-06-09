// Native oracle for the recursive Node tree.
#include "tree.h"
#include <cstdio>
int main() {
    tree::Node root(10);
    tree::Node a(3), b(5);
    a.add(tree::Node(1));            // a has one child -> a.total()=4
    root.add(a);                    // copies a (with its child) into root
    root.add(b);
    std::printf("{ \"root_total\": %d, \"root_count\": %ld, \"a_total\": %d, \"child0_value\": %d }\n",
                root.total(), root.count(), a.total(), root.children[0].value);
    return 0;
}
