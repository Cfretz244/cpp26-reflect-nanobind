// TC-0013 candidate: dependent splice as auto-NTTP argument inside a
// requires type-requirement ICEs at parse (DeduceAutoType -> ClassifyInternal).
#include <experimental/meta>
template <auto> struct probe;
template <std::meta::info mem>
bool f() {
    if constexpr (requires { typename probe<([:mem:])>; })
        return true;
    return false;
}
struct S { static const int K = 7; };
bool b = f<^^S::K>();
