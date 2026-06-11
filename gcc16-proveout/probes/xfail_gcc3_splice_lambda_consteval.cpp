// GCC-3 (EXPECTED TO FAIL on GCC 16.1; divergence repro): a lambda whose
// BODY splices an enclosing info NTTP is treated as consteval-only, so its
// conversion to a plain function pointer is an immediate-function-address
// error ("immediate evaluation returns address of immediate function").
// clang-p2996 accepts this. Hoisting the splice to a constexpr
// pointer-to-member outside the lambda makes it portable.
//
// The binder's workaround: reflect_bind_conversion hoists `&[:fn:]`
// (nb_reflect.h).
#include <meta>
#include <cstdio>

struct Ops { explicit operator bool() const { return true; } };

using namespace std::meta;

consteval info conv(info cls) {
    for (info m : members_of(cls, access_context::unchecked()))
        if (is_conversion_function(m)) return m;
    return info{};
}

template <info fn, typename T>
void bind(T& t) {
    auto lam = [](T& self) -> bool { return (bool) self.[:fn:](); };
    bool (*fp)(T&) = lam;     // GCC: lam's _FUN is an immediate function
    printf("via fp: %d\n", fp(t));
}

int main() {
    Ops o;
    bind<conv(^^Ops), Ops>(o);
}
