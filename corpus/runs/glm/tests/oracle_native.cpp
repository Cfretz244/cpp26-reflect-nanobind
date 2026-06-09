// Native oracle for the glm vec subset: construct vec3/vec4, index them, query length().
#include <glm/glm.hpp>
#include <cstdio>

int main() {
    glm::vec3 a(1.5f, 2.5f, 3.5f);
    glm::vec3 b(4.0f);                       // scalar broadcast
    glm::vec4 c(1.0f, 2.0f, 3.0f, 4.0f);

    std::printf(
        "{\n"
        "  \"a0\": %.9g, \"a1\": %.9g, \"a2\": %.9g,\n"
        "  \"b0\": %.9g, \"b1\": %.9g, \"b2\": %.9g,\n"
        "  \"c0\": %.9g, \"c1\": %.9g, \"c2\": %.9g, \"c3\": %.9g,\n"
        "  \"len3\": %d, \"len4\": %d\n"
        "}\n",
        (double)a[0], (double)a[1], (double)a[2],
        (double)b[0], (double)b[1], (double)b[2],
        (double)c[0], (double)c[1], (double)c[2], (double)c[3],
        (int)glm::vec3::length(), (int)glm::vec4::length());
    return 0;
}
