// Mirrors what nb_reflect.h entity_name<^^T>() does for a C-style typedef'd
// anonymous struct passed as a template type parameter.
#include <experimental/meta>

typedef struct { int x; int y; } point_t;   // the tinyobj idiom

template <typename T>
consteval const char* entity_name() {
    // identifier_of(^^T) is NOT a constant expression when T resolves to an
    // anonymous record named only through a typedef:
    return std::define_static_string(std::meta::identifier_of(^^T));
}

template <typename T>
void reflect_class() {
    constexpr auto name = entity_name<T>();   // <-- error here
    (void)name;
}

int main() { reflect_class<point_t>(); }
