// Workaround validation for probe 09's two failures:
//  (a) gate type predicates behind is_type (GCC throws on non-type args)
//  (b) splice inside info-NTTP helper templates, never at the dispatch site,
//      so discarded if-constexpr branches contain only dependent constructs.
#include <meta>
#include <cstdio>

namespace api {
struct C { int x; };
enum class E { A };
void fn() {}
namespace inner {}
} // namespace api

using namespace std::meta;

template <info M> void handle_class() {
    using T = typename [:M:];                  // dependent: only checked if instantiated
    printf("class %s (size %zu)\n", identifier_of(M).data(), sizeof(T));
}
template <info M> void handle_enum() {
    using T = typename [:M:];
    printf("enum %s\n", identifier_of(M).data());
}
template <info F> void handle_fn() { printf("fn %s\n", identifier_of(F).data()); }

enum class kind { cls, en, fn, skip };
consteval kind classify(info m) {
    if (is_type(m)) {                          // gate FIRST: is_class_type throws on non-types
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
        if constexpr (k == kind::cls)      handle_class<mem>();
        else if constexpr (k == kind::en)  handle_enum<mem>();
        else if constexpr (k == kind::fn)  handle_fn<mem>();
    }
}

int main() {
    dispatch<^^api>();
    printf("09b_workaround PASS\n");
}
