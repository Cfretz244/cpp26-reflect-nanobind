dedup_key: mangle-reflection-deduction-guide-function-encoding-unreachable

# Deduction-guide reflection ICEs the Itanium mangler via mangleFunctionEncoding (ItaniumMangle.cpp:1774)

Layer: TOOLCHAIN (clang-p2996 Itanium mangler)

## Smallest trigger
A reflection whose value is (or transitively names) a class-template **deduction guide** is
mangled as a non-type template argument and routed through `mangleFunctionEncoding` ->
`mangleName` -> `mangleNestedName` -> `mangleUnqualifiedName`, where the
`DeclarationName::CXXDeductionGuideName` case is `llvm_unreachable("Can't mangle a deduction
guide name!")`. Found in the simdjson DOM corpus run: when the binder's bind-set fixpoint
reached `simdjson::arm64::ondemand::field` (`class field : public std::pair<raw_json_string,
value>`) and that type was placed in an `nb::exclude_<...>` pack, instantiating the exclude
machinery mangled a reflection that reached the per-`field` deduction guides.

(How the bind set reached `ondemand::field` at all was a SEPARATE, run-side cause: a reflected
fixture namespace declared a namespace-alias member `namespace sd = simdjson;`, and the
namespace walk followed the alias into the whole library. That is fixed run-side by moving the
aliases out of the reflected namespace. The ICE below is the toolchain robustness gap that the
leak exposed.)

## Diagnostics (actual)
```
Can't mangle a deduction guide name!
UNREACHABLE executed at .../clang/lib/AST/ItaniumMangle.cpp:1774!
 #8  CXXNameMangler::mangleUnqualifiedName(...)
 #9  CXXNameMangler::mangleNestedName(...)
 #10 CXXNameMangler::mangleFunctionEncoding(GlobalDecl)
 #11 CXXNameMangler::mangleReflection(APValue const&)
 #12 CXXNameMangler::mangleValueInTemplateArg(QualType, APValue const&, bool, bool)
 #13 CXXNameMangler::mangleTemplateArg(TemplateArgument, bool)
 ...
 #19 getMangledNameImpl(CodeGenModule&, GlobalDecl, NamedDecl const*, bool)
 #21 CodeGenModule::EmitGlobal(GlobalDecl)
 #26 MetaActionsImpl::EnsureInstantiated(Decl*, SourceRange)
```
clang frontend command failed with exit code 134.

## Why this is a NEW path vs TC-0008
TC-0008 (issue #298 / PR #299) made deduction-guide REFLECTIONS mangle as `"dg" + deduced
template + ODR hash` inside `mangleReflection`'s declaration short-circuit. This crash is in a
DIFFERENT branch: `mangleReflection` here routes the guide's reflection through
`mangleFunctionEncoding` -> `mangleName` -> `mangleNestedName` -> `mangleUnqualifiedName`, i.e.
it mangles the guide *as an ordinary function entity by name*, hitting the
`CXXDeductionGuideName` `llvm_unreachable` rather than the TC-0008 short-circuit. The binder's
namespace-walk guide-stripping (`namespace_members_for_binding`) also did not apply because the
guide was reached through the exclude_/discovery machinery on a class type, not a namespace lift.

## Status in THIS run
Not hit by the committed run: the fixture no longer leaks the library (namespace aliases moved
to global scope), so the bind set never reaches `ondemand::field` and the mangler is never asked
to mangle a guide. The run lands E without touching this path. Repro retained at
`findings_draft/ice_repro.cpp` (compile with the run's binding flags; EXPECTED clean/graceful,
ACTUAL exit 134 + the UNREACHABLE above).

## Suggested fix direction (for triage, not applied here)
Mirror the TC-0008 treatment in the `mangleFunctionEncoding`/`mangleUnqualifiedName` path: when
the function decl is a `CXXDeductionGuideDecl`, emit a stable synthetic name (e.g. `"dg"` + the
deduced template + an ODR/qualifier hash) instead of `llvm_unreachable`, so a guide reflection
that arrives via function-encoding mangles deterministically rather than aborting.
