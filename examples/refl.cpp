// refl.cpp — build with: clang++ -std=c++26 -freflection-latest -stdlib=libc++ ...
#include <experimental/meta>
#include <print>

using namespace std::meta;

struct Point { int x; int y; };

int main() {
  // `std::meta::info` is a consteval-only type, so all reflection happens in a
  // constant-evaluated context. `define_static_array` lifts the (heap) vector of
  // reflections into static storage so a `template for` can expand over it.
  template for (constexpr auto member :
                define_static_array(
                    nonstatic_data_members_of(^^Point, access_context::current()))) {
    std::println("Point.{} : {}", identifier_of(member),
                 display_string_of(type_of(member)));
  }
}
