// GCC-6 (EXPECTED TO FAIL on GCC 16.1; minimized bug repro): lifting the
// reflection of a `constexpr` (or consteval) member function into
// define_static_array instantiates that member's DEFINITION -- GCC has to
// decide whether the function is usable in constant evaluation to materialize
// its reflection as an NTTP, and that decision instantiates the body. For a
// member whose constexpr body is LAZILY ill-formed in this specialization the
// lift itself becomes a hard error, even though nothing ever calls the member.
//
// Field shape: tl::expected<void, E>'s `constexpr operator->() const` returns
// `valptr()`, which is `addressof(this->m_val)`; the void storage base has no
// `m_val`, so the body is ill-formed only when instantiated for the void
// specialization. Reflecting that specialization to bind it (members_of -> the
// define_static_array lift in nb_reflect.h's liftable_class_members) tripped
// the hard error. Note the contrast below: only the CONSTEXPR member bites; a
// non-constexpr member with the identical ill-formed body is NOT instantiated
// by the lift. clang-p2996 never instantiates a body on lift, so it binds the
// void specialization cleanly -- this divergence is GCC's.
//
//   $ g++ -std=c++26 -freflection -fsyntax-only xfail_gcc6_constexpr_member_lift.cpp
//   expected: clean (the lift only materializes reflections; bodies untouched)
//   actual:   error: 'const struct S<void>' has no member named 'm_val',
//             required from 'constexpr ... S<T>::cefn() const [with T = void]',
//             required from the define_static_array lift below
//
// Binder workaround: never_bound_plain_member_fn (nb_reflect.h) drops members
// that no binding pass consumes (non-public functions, and member operators
// with no Python dunder -- operator-> / unary operator* / ...) BEFORE the lift,
// keeping their lazily-ill-formed constexpr bodies out of static storage. It
// does NOT (and cannot, without instantiating) drop a mapped/ordinary member
// whose constexpr body is lazily ill-formed; no such member arises in practice
// because libraries enable_if those away (they are absent from members_of).
#include <meta>
#include <vector>
#include <memory>      // std::addressof
#include <cstdio>

template <class T> struct Base { T m_val; };   // general storage: has m_val
template <> struct Base<void> {};              // void storage: no m_val

template <class T>
struct S : Base<T> {
    int ok = 0;
    // Lazily ill-formed for T=void: this->m_val does not exist there. Dependent,
    // so the error surfaces only on instantiation (mirrors expected::valptr).
    constexpr const T* cefn() const { return std::addressof(this->m_val); }
    // Same ill-formed body, but NON-constexpr: the lift does NOT instantiate it.
    const T* plainfn() const { return std::addressof(this->m_val); }
};

using namespace std::meta;

int main() {
#if 0   // fine: enumeration alone never instantiates the constexpr body
    constexpr size_t n = members_of(^^S<void>, access_context::unchecked()).size();
    printf("count=%zu\n", n);
#endif
    // hard error on GCC 16.1: instantiating S<void>::cefn()'s body (required to
    // materialize its reflection as an NTTP inside the lift) references a
    // nonexistent m_val. plainfn(), with the identical body but no constexpr,
    // is materialized without instantiation -- so constexpr-ness is the trigger.
    constexpr auto ms = define_static_array(
        members_of(^^S<void>, access_context::unchecked()));
    printf("lifted=%zu\n", ms.size());
}
