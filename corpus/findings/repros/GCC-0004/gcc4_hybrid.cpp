// Probe-09 residual: conforming classify (is_type gated), but splices kept
// AT THE DISPATCH SITE inside discarded if-constexpr branches of the
// expanded template-for body. Does GCC reject the discarded splices?
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

enum class kind { cls, en, fn, skip };
consteval kind classify(info m) {
    if (is_type(m)) {
        if (is_class_type(m)) return kind::cls;
        if (is_enum_type(m)) return kind::en;
        return kind::skip;
    }
    if (is_function(m)) return kind::fn;
    return kind::skip;
}

template <info Ns>
void dispatch() {
    template for (constexpr auto mem : define_static_array(
                      members_of(Ns, access_context::unchecked()))) {
        constexpr kind k = classify(mem);
        if constexpr (k == kind::cls)
            handle_class<typename [:mem:]>();   // splice at site, discarded for fn
        else if constexpr (k == kind::en)
            handle_enum<typename [:mem:]>();
        else if constexpr (k == kind::fn)
            handle_fn<mem>();
    }
}

int main() {
    dispatch<^^api>();
    printf("hybrid PASS\n");
}
