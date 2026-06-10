Fixes #ISSUE_TC7.

## Problem

`SemaMetaActions::EnsureInstantiated` — the single completion funnel for every type-completing metafunction (`members_of`, `bases_of`, `size_of`, `is_complete_type`, ...) — completed class template specializations with `TSK_ExplicitInstantiationDefinition` **plus** `InstantiateClassTemplateSpecializationMembers`, eagerly instantiating every member *definition*. A specialization whose never-odr-used member bodies are ill-formed (the "specialized storage base" idiom — valid C++; `tl::expected<void, E>` is the field shape) was wrong-rejected, **order-dependently**: a preceding ordinary use completed the class lazily first and the identical `members_of` loop then compiled clean (see #ISSUE_TC7 for the reproducer + `-DPREINSTANTIATE` control). The eager kind also marked the specialization as a user-written explicit instantiation definition — weak-ODR emission of every member in the TU, and collisions with genuine `template struct ...;` declarations.

## Fix

`clang/lib/Sema/SemaReflect.cpp`, `EnsureInstantiated`: mirror `Sema::RequireCompleteTypeImpl` —

- `InstantiateClassTemplateSpecialization(..., TSK_ImplicitInstantiation, /*Complain=*/false, CTSD->hasStrictPackMatch())`;
- drop the `InstantiateClassTemplateSpecializationMembers` sweep entirely (a reflection query needs completeness — layout, bases, declared members — never member bodies);
- add the member-class-of-a-template branch (`InstantiateClass` on `getInstantiatedFromMemberClass`, `TSK_ImplicitInstantiation`) that the eager member sweep used to cover as a side effect, so `members_of(^^Outer<T>::Inner)` keeps working when reflection completes `Inner` on demand.

Body-needing consumers (`extract`, `reflect_invoke`, `substitute` of deduced-return functions) re-enter `EnsureInstantiated` with the specific `FunctionDecl`/`VarDecl`; those branches are untouched, so constant-evaluating a member of an implicitly-instantiated specialization still instantiates its body on demand.

## Test

`libcxx/test/std/experimental/reflection/members-of-lazily-ill-formed-bodies.pass.cpp` (new): the specialized-storage-base idiom with five distinct templates so each metafunction (`members_of`, `nonstatic_data_members_of`, `bases_of`, `is_complete_type`, `size_of`) is the *first* instantiation trigger for its specialization; an order-independence control (ordinary use first → identical enumeration); and a runtime check that odr-used bodies still instantiate lazily.

## Validation

- Base: `837da39eb88c` (current `p2996` tip; applies cleanly, builds standalone).
- Without the fix: the #ISSUE_TC7 reproducer errors (`no member named 'm_val' in 'Exp<void>'` from inside the `members_of` iteration) and is order-dependent; the new test wrong-rejects.
- With the fix: reproducer clean in both orderings; new test passes.
- `clang/test/Reflection`: 16/16 with the fix on this machine — identical to base. Body-needing suite spot-checks green: `reflect-invoke.pass.cpp`, `to-and-from-values.pass.cpp`, `consteval-reentrant-instantiation.pass.cpp`, `members-and-subobjects.pass.cpp`.
- Downstream soak (reflection-driven nanobind binding generator): 53-test binder suite green; TartanLlama/expected v1.3.1 corpus run with `tl::expected<void, std::string>` in the bind set passes its full differential suite (previously: nine hard errors from tl's implementation details on bare enumeration).

🤖 Generated with [Claude Code](https://claude.com/claude-code)
