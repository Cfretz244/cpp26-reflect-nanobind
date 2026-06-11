// Does GCC check splices in DISCARDED if-constexpr branches inside an
// expanded `template for` body? This is the binder's central dispatch idiom
// (nb_reflect.h reflect_dispatch): if it fails, every dispatch site needs a
// dependent-context wrapper.
#include <meta>
#include <cstdio>

namespace api {
struct C { int x; };
enum class E { A };
void fn() {}
} // namespace api

using namespace std::meta;

template <typename T> void handle_class() { printf("class\n"); }
template <typename T> void handle_enum()  { printf("enum\n"); }
template <info F>     void handle_fn()    { printf("fn\n"); }

enum class kind { cls, en, fn };
consteval kind classify(info m) {
    if (is_class_type(m)) return kind::cls;
    if (is_enum_type(m)) return kind::en;
    return kind::fn;
}

// The binder's exact shape: an enclosing template, template for over the
// namespace, if constexpr routing with splices in each branch.
template <info Ns>
void dispatch() {
    template for (constexpr auto mem : define_static_array(
                      members_of(Ns, access_context::unchecked()))) {
        constexpr kind k = classify(mem);
        if constexpr (k == kind::cls)
            handle_class<typename [:mem:]>();   // splice invalid for fn/enum? discarded!
        else if constexpr (k == kind::en)
            handle_enum<typename [:mem:]>();
        else if constexpr (k == kind::fn)
            handle_fn<mem>();
    }
}

int main() {
    dispatch<^^api>();
    printf("09_discarded_splice PASS\n");
}
