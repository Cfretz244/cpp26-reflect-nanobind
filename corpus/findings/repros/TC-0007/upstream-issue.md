## Summary

When a type-completing metafunction (`members_of`, `bases_of`, `size_of`, `is_complete_type`, ...) is the **first** thing to instantiate a class template specialization, the specialization is completed with `TSK_ExplicitInstantiationDefinition` **plus** `InstantiateClassTemplateSpecializationMembers` — i.e. every member *definition* is eagerly instantiated, as if the user had written `template struct Exp<void>;`. Normal C++ instantiates a member's definition only when it is odr-used; a specialization whose member bodies are ill-formed-but-never-used (the "specialized storage base" idiom) is valid C++, and reflection wrong-rejects it.

The smoking gun is the **order dependence**: adding one ordinary use *before* the probe makes the identical `members_of` loop compile clean (the class then got completed with normal lazy semantics first, and enumerating an already-complete class instantiates nothing). Whether reflection succeeds on a type should not depend on whether unrelated earlier code happened to instantiate it.

The eager kind has a second cost even for well-formed bodies: the specialization is marked as an explicit instantiation definition, forcing weak-ODR emission of every member in the TU and colliding with a later genuine `template struct ...;` (duplicate-explicit-instantiation diagnostics).

## Field evidence

`tl::expected<void, std::string>` (TartanLlama/expected v1.3.1): a bare `members_of(^^tl::expected<void, std::string>, access_context::unchecked())` loop — no substitution, just enumeration — produces nine hard errors out of tl's implementation details (`valptr()`-style private helpers referencing `this->m_val` and the `enable_if`'d `val()`, valid for every `T` except `void` and never instantiated for `void` by ordinary use).

## Reproducer

Self-contained, `-std=c++26 -freflection-latest`, `-fsyntax-only` suffices:

```cpp
#include <experimental/meta>

template <class T> struct storage { T m_val; };
template <> struct storage<void> { char m_dummy; };

template <class T> struct Exp : private storage<T> {
  T* valptr() { return &this->m_val; }  // body valid for every T except void
  bool has_value() const { return true; }
};

#ifdef PREINSTANTIATE
Exp<void> ok_instance;  // control: ordinary use first => identical loop compiles
#endif

consteval int n(std::meta::info cls) {
  int k = 0;
  for (auto m : std::meta::members_of(cls, std::meta::access_context::unchecked()))
    ++k;
  return k;
}

static_assert(n(^^Exp<void>) > 0);

int main() {}
```

- default: `error: no member named 'm_val' in 'Exp<void>'` + note pointing into the `members_of` iteration (the bug)
- `-DPREINSTANTIATE`: compiles clean (the order-dependence control)

## Expected

Compiles in both configurations; `members_of` enumerates the member *declarations* (a reflection query needs completeness — layout, bases, declared members — never member bodies; consumers that do need a body, like `extract`/`reflect_invoke`, already request instantiation of the specific function on demand).

## Actual (at `837da39eb88c`)

```
error: static assertion expression is not an integral constant expression
note: in call to 'n(^^Exp<void>)'
error: no member named 'm_val' in 'Exp<void>'
```

(only without `-DPREINSTANTIATE` — order-dependent).

## Suggested fix (PR follows shortly)

`SemaMetaActions::EnsureInstantiated` is the single completion funnel for every type-completing metafunction. Mirror `Sema::RequireCompleteTypeImpl`: complete the specialization with `TSK_ImplicitInstantiation` (and its strict-pack-match flag), drop the `InstantiateClassTemplateSpecializationMembers` sweep, and add the member-class-of-a-template branch (`InstantiateClass` on `getInstantiatedFromMemberClass`) that the eager member sweep used to cover as a side effect. Body-needing consumers re-enter `EnsureInstantiated` with the specific `FunctionDecl`/`VarDecl`, whose branches are untouched. Regression test included with the PR.
