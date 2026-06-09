# BINDER-0002 — Unnamed (anonymous-union) data members hard-error

- **Status:** PARTIAL FIX (binder skips them) + documented limitation
- **Found via:** Phase 1, g-truc/glm `vec3`/`vec4` (outcome B → E for the indexing/ctor surface)
- **File:** `nanobind/include/nanobind/nb_reflect.h`, `bind_class_contents` data-member loops

## Symptom

`nb::reflect_<^^glm::vec3, ^^glm::vec4>(m)` failed to compile:

```
error: constexpr variable 'name' must be initialized by a constant expression
   constexpr auto name = entity_name<mem>();      // nb_reflect.h:334
note: in call to 'identifier_of(^^(declaration))'
```

glm stores components in an **anonymous union** for swizzle aliasing:

```cpp
union { struct { T x,y,z,w; }; struct { T r,g,b,a; }; struct { T s,t,p,q; }; };
```

`nonstatic_data_members_of(vec)` yields the *unnamed* union member; `identifier_of()` on it is
ill-formed, so the binder hard-errored.

## Fix (partial)

Guard the data-member loops with `std::meta::has_identifier(mem)` — unnamed members are
gracefully skipped (matching the volatile/&&/template-member handling). glm vec now binds its
**constructors**, **`operator[]`→`__getitem__`**, and static **`length()`**, which is a usable,
testable surface (corpus/runs/glm, outcome E).

## Remaining limitation (not fixed)

The named components `x/y/z/w` are **not exposed**. Exposing them is not a simple loop guard:
- A pointer-to-member of the enclosing class cannot be formed for an anonymous-union member
  (`&glm::vec3::x` is ill-formed), so the `def_rw(&T::x)` path is impossible.
- It would require `def_prop` getter/setter lambdas that access `self.x` by splice — which runs
  into the spliced-lambda mangler-crash gotcha and needs care.

Roadmap item: flatten anonymous union/struct members via property accessors. Until then, glm-style
swizzle vectors are usable by index but not by component name.
