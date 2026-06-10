// TC-0007 — members_of, when IT is what first instantiates a class template
// specialization, instantiates member function DEFINITIONS too; a valid type
// whose member bodies are lazily ill-formed is wrong-rejected.
// Standalone repro (no nanobind, no library). See
// corpus/findings/TC-0007-members-of-instantiates-member-bodies.md.
//
// Ordinary C++ instantiates a class template member's definition only when it
// is odr-used; a member whose BODY is ill-formed for some specialization is
// fine as long as that specialization never calls it (the entire
// "specialized storage base" idiom relies on this). Enumerating the members
// of such a specialization with std::meta::members_of should yield the member
// DECLARATIONS (completing the class with the usual implicit-instantiation
// semantics); in clang-p2996, when the members_of call is the FIRST thing to
// instantiate the specialization, the member definitions are instantiated
// too, turning a valid program into a hard error.
//
// Default build -- the bug. Expected: compiles (the static_assert holds).
// Actual at clang-p2996 (toolchain pinned by this repo, base 837da39eb88c
// family):
//   error: no member named 'm_val' in 'Exp<void>'
//   note: in instantiation of member function 'Exp<void>::valptr' requested here
// pointing into the members_of iteration (<meta>'s lazy member-range m_next).
//
// -DPREINSTANTIATE -- the control AND the inconsistency: an ordinary use of
// Exp<void> before the probe (a variable definition) instantiates the class
// lazily, after which the IDENTICAL members_of loop compiles clean. The
// enumeration result may not depend on whether unrelated earlier code
// happened to instantiate the type.
//
// Field shape: tl::expected<void, std::string> (TartanLlama/expected v1.3.1).
// tl::expected keeps its value in a void-specialized storage base; the
// primary template's private valptr()/swap helpers reference this->m_val and
// call val(), valid for every T except void and never instantiated for void
// by ordinary use. A bare members_of(^^tl::expected<void, std::string>,
// access_context::unchecked()) loop produces 9 hard errors (valptr,
// swap_where_both_have_value, swap_where_only_one_has_value_and_t_is_not_void),
// so the reflection-driven nanobind binder cannot even LOOK at the void
// specialization. The non-void specs enumerate fine (their bodies happen to
// instantiate cleanly).
//
// Build (from the umbrella repo root; -fsyntax-only suffices):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 -fsyntax-only repro.cpp [-DPREINSTANTIATE]
#include <experimental/meta>

namespace meta = std::meta;

template <class T> struct storage { T m_val; };
template <> struct storage<void> { char m_dummy; };

template <class T> struct Exp : private storage<T> {
  T* valptr() { return &this->m_val; }  // body valid for every T except void
  bool has_value() const { return true; }
};

#ifdef PREINSTANTIATE
// Ordinary lazy instantiation first: Exp<void> is a perfectly valid type
// (valptr() is never odr-used), and afterwards the probe below compiles.
Exp<void> ok_instance;
#endif

consteval int count_members(meta::info cls) {
  int n = 0;
  for (auto mem : meta::members_of(cls, meta::access_context::unchecked()))
    ++n;
  return n;
}

static_assert(count_members(^^Exp<void>) > 0);

int main() {}
