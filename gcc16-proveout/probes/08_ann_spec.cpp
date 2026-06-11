// Isolate the binder build failure: annotations_of on function-template
// SPECIALIZATIONS, and the two-arg annotations_of(R, ^^A) filtered form.
#include <meta>
#include <cstdio>

namespace ann { struct tag_t {}; inline constexpr tag_t tag{}; }

namespace api {
[[=ann::tag]] int plain_fn(int x) { return x; }
template <typename T> [[=ann::tag]] T identity(T v) { return v; }
struct S { [[=ann::tag]] int field; };
} // namespace api

using namespace std::meta;

int main() {
    // 1. two-arg form on a plain function
    constexpr auto a1 = annotations_of(^^api::plain_fn, ^^ann::tag_t);
    static_assert(!a1.empty());
    printf("two-arg on plain function: OK\n");

    // 2. one-arg form on a function-template specialization
    constexpr auto spec = substitute(^^api::identity, {^^int});
    constexpr auto a2 = annotations_of(spec);
    printf("one-arg on fn-template spec: OK (count=%zu)\n", a2.size());

    // 3. two-arg form on the same specialization  <-- suspected failure
    constexpr auto a3 = annotations_of(spec, ^^ann::tag_t);
    printf("two-arg on fn-template spec: OK (count=%zu)\n", a3.size());

    // 4. two-arg form on a NON-annotated entity (the has_ann common path)
    constexpr auto a4 = annotations_of(^^api::S, ^^ann::tag_t);
    static_assert(a4.empty());
    printf("two-arg on unannotated: OK\n");

    printf("08_ann_spec PASS\n");
}
