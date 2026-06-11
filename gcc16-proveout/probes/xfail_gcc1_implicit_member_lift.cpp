// GCC-1 (EXPECTED TO FAIL on GCC 16.1; minimized bug repro): lifting the
// reflection of an IMPLICITLY-declared special member into
// define_static_array (i.e. materializing it as an NTTP) instantiates that
// member's DEFINITION. For a class whose member is copyable-by-declaration
// but ill-formed to instantiate (vector<unique_ptr<T>>), the lift itself is
// a hard error. members_of alone (no lift) is fine -- see the #if 0 block.
//
// The binder's workaround: liftable_class_members (nb_reflect.h) drops
// implicit copy/move ctors + assignment operators before every lift.
#include <meta>
#include <memory>
#include <vector>
#include <cstdio>

struct Item { int v = 7; };
// No user-declared members: implicit default/copy/move ctors + assignments.
struct Bag { std::vector<std::unique_ptr<Item>> items; };

using namespace std::meta;

int main() {
#if 0   // fine: enumeration alone does not instantiate implicit definitions
    constexpr size_t n = members_of(^^Bag, access_context::unchecked()).size();
    printf("count=%zu\n", n);
#endif
    // hard error on GCC 16.1: use of deleted unique_ptr copy ctor/operator=,
    // required from the implicit Bag copy ctor / operator= DEFINITIONS,
    // required from this lift
    constexpr auto ms = define_static_array(
        members_of(^^Bag, access_context::unchecked()));
    printf("lifted=%zu\n", ms.size());
}
