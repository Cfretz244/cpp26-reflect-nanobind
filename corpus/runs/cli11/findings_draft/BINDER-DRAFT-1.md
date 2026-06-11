dedup_key: emit-spell-wchar-char-family-display-string-underlying
layer: BINDER (emitter / nb_reflect_spell.h)

# Emitter spelled `wchar_t` as `int`, miscompiling `std::wstring` signatures

## Symptom

The cli11 emit lane's stage-2 (production-compiler) build of the generated
source failed:

```
binding.gen.cpp:158: error: no matching member function for call to 'parse'
  ... ::std::basic_string<int, ::std::char_traits<int>, ::std::allocator<int>> ...
note: candidate function not viable: no known conversion from
  'basic_string<int, char_traits<int>, allocator<int>>'
  to 'basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>>'
```

CLI11's `App::parse(std::wstring, bool)` overload reflects into the bind set;
the emitter rendered its `std::wstring` parameter as
`std::basic_string<int, ...>` (the forwarding lambda then can't call the real
`parse(std::wstring)`). The constexpr lane binds the same overload fine -- the
divergence is purely in the emitter's TEXT rendering.

## Root cause

`nb_reflect_spell.h`'s fundamental-type fallback (`type_spelling`) ended with
`return std::string(std::meta::display_string_of(t))`. On this toolchain
`display_string_of(^^wchar_t)` yields `"int"` -- `wchar_t`'s UNDERLYING type,
not its own keyword (confirmed by direct probe; char8/16/32_t display
correctly, but are equally identity-distinct fundamentals worth pinning). So
any signature mentioning `wchar_t` (here through `std::wstring =
basic_string<wchar_t>`) emitted the wrong element type and failed to match the
library overload. A toolchain display-string quirk, surfaced as an emitter
correctness bug.

## Fix

In `type_spelling`, before the `display_string_of` fallback, spell the
char-family fundamentals by type identity so the keyword is always exact:

```cpp
if (std::meta::dealias(t) == ^^wchar_t)  return "wchar_t";
if (std::meta::dealias(t) == ^^char8_t)  return "char8_t";
if (std::meta::dealias(t) == ^^char16_t) return "char16_t";
if (std::meta::dealias(t) == ^^char32_t) return "char32_t";
```

Right layer: text rendering only -- the binder's WHAT-to-bind decision is
unchanged, both backends still bind the same overloads. No emitter/classifier
fork. (`wchar_t` was the only observed offender; the other three are pinned
defensively because their spelling correctness must not depend on
`display_string_of` either.)

## Files touched

- `nanobind/include/nanobind/nb_reflect_spell.h` -- the char-family identity
  arms in `type_spelling`.
- `nanobind/tests/test_reflect.cpp` -- static_asserts on `type_spelling` for
  the char family + `std::wstring` (the bug class is representable as a
  compile-time spelling check).

## Verification

- Reflection suite (toolchain): test_reflect.py + test_reflect_codegen.py +
  test_reflect_emit.py = **131 passed**; the new static_asserts compile.
- cli11 three-way gate: `outcome=E constexpr=E emit=E surface=pass`. The
  generated `binding.gen.cpp` now spells `parse(std::wstring)` as
  `basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>>` and
  compiles under Apple Clang.
- Constexpr lane unchanged (E, == pre-wave result.json).
