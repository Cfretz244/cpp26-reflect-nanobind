# BINDER-0003 — Spec name renders enum non-type template args as "Unsupportedreflection"

- **Status:** open (cosmetic; suspected toolchain `display_string_of` gap)
- **Found via:** Phase 1, glm `vec3` → Python class name `vec3FloatUnsupportedreflection`
- **File:** `nanobind/include/nanobind/nb_reflect.h`, `spec_camel_name` (non-type arg branch)

## Symptom

`glm::vec3` is `glm::vec<3, float, glm::qualifier::packed_highp>`. The CamelCase Python-name
generator renders the third argument (an enum non-type template parameter, value
`glm::qualifier::packed_highp`) as the literal `Unsupportedreflection`, giving the class name
`vec3FloatUnsupportedreflection` (likewise `vec4FloatUnsupportedreflection`).

## Cause

`spec_camel_name` renders a non-type arg via `sanitize_identifier(display_string_of(arg))`. For an
enum-typed NTTP **value**, this toolchain's `display_string_of` returns a placeholder string
(sanitized to `Unsupportedreflection`) rather than the enumerator name. So this is partly a
toolchain limitation and partly a binder one (it has no enum-aware fallback).

## Impact

Cosmetic only — the binding is fully functional; the class is reachable via `getattr`. But the
name is unusable for ergonomic Python code. Affects any library whose template specializations
carry enum NTTPs (glm qualifiers, many policy-parameterized templates).

## Possible fix

In `spec_camel_name`'s non-type branch: if `type_of(arg)` is an enumeration, find the enumerator
whose value matches and emit its identifier (e.g. `PackedHighp`), falling back to the current
behavior. Verify `display_string_of` / enumerator enumeration works for enum NTTP constants on the
pinned toolchain first (may need a toolchain-track minimization).
