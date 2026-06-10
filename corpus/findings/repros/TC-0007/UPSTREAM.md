# DRAFT — not yet filed (bloomberg/clang-p2996)

Candidate issue title:
**`members_of` instantiates member function definitions when it triggers the
class template specialization's instantiation — valid types with
lazily-ill-formed member bodies are wrong-rejected (order-dependent)**

## Summary

When a `std::meta::members_of` call is the first thing to instantiate a class
template specialization, the members' *definitions* are instantiated — not just
the declarations the enumeration needs. A specialization whose member bodies
are ill-formed-unless-used (the "specialized storage base" idiom: a body
referencing `this->m_val` where the `void` storage specialization has no
`m_val`) is a perfectly valid type in ordinary C++ but hard-errors under
enumeration.

The behavior is order-dependent, which is the smoking gun: adding one ordinary
use (`Exp<void> ok_instance;`) before the identical `members_of` loop makes the
program compile — normal lazy completion happened first, and enumerating an
already-instantiated class does not re-instantiate bodies. Reflection appears
to complete the class with explicit-instantiation-definition-like eagerness
instead of plain implicit instantiation.

## Repro

`repro.cpp` in this directory; `-fsyntax-only` suffices:

```
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -fsyntax-only repro.cpp                    # error (the bug)
clang++ -std=c++26 -freflection-latest -stdlib=libc++ -fsyntax-only -DPREINSTANTIATE repro.cpp   # clean (control)
```

Default build, actual:

```
error: no member named 'm_val' in 'Exp<void>'
note: in instantiation of member function 'Exp<void>::valptr' requested here
  (from <meta>'s member-range iterator m_next)
```

## Field shape

`tl::expected<void, std::string>` (TartanLlama/expected v1.3.1, ships
`expected<void, E>` since 2017): a bare
`members_of(^^tl::expected<void, std::string>, access_context::unchecked())`
loop produces 9 hard errors out of tl's private `valptr` /
`swap_where_both_have_value` / `swap_where_only_one_has_value_and_t_is_not_void`
helpers — all valid-for-every-`T`-except-`void` bodies that ordinary use never
instantiates for `void`. Pre-instantiating the spec dodges this finding but not
the companion `can_substitute` SEGV (TC-0006 in this corpus), which hits the
same type's `swap` member template.
