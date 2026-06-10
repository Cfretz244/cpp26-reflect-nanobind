dedup_key: binder-ptr-to-ptr-param-no-graceful-skip
layer: BINDER

# A method with a pointer-to-pointer parameter hard-errors the binder instead of skipping

## Smallest trigger

A reflected method whose parameter (or return) is a pointer-to-pointer (`char**`,
`const char* const*`, `wchar_t**`, ...). Minimal standalone repro (`/tmp/ptrptr.cpp`):

```cpp
#include <nanobind/nb_reflect.h>
namespace nb = nanobind;
namespace mini {
struct Argv {
    Argv() = default;
    char** mangle(char** argv) { return argv; }   // pointer-to-pointer param
    int ok() const { return 1; }
};
}
NB_MODULE(ptrptr, m){ nb::reflect_<^^mini::Argv>(m); }
```

`-fsyntax-only` fails:

```
nb_func.h:292: error: no matching function for call to object of type
  'const ... (lambda at nb_reflect.h:396:1)'
nb_reflect.h:396: note: candidate function not viable: no known conversion from
  'const char *' to 'char **' for 2nd argument
```

## Diagnosis

`char**` has no nanobind type caster. The binder gracefully skips many non-bindable shapes
(volatile / rvalue-ref-qualified / C-variadic functions, anonymous-union and C-array data
members, deleted functions). A pointer-to-pointer *parameter* is not on that skip list: the
binder synthesizes the forwarding method lambda anyway, and `nb_func.h` then fails to match it
because the argument cannot be converted -- a hard compile error that takes down the whole
`reflect_<...>` translation unit. The expected behavior is the same graceful skip as the other
uncastable shapes (drop the one method, keep the class).

## Field shape (CLI11 v2.6.2)

`CLI::App` declares `char** ensure_utf8(char** argv)` and the argv-style overloads
`parse(int, const char* const*)` / `parse(int, const wchar_t* const*)`. With `App` in the
reflect pack these three members hard-error the binder; the cli11 run works around it by
excluding exactly the pointer-to-pointer members via a `nb::exclude_` marker built from
`std::meta` (`has_ptr_to_ptr_param`). The bindable `parse(std::vector<std::string>&)` overload
is the one the run actually drives.

## First diagnostics

```
nb_func.h:292: error: no matching function for call to object of type '... lambda ...'
nb_reflect.h:396: note: candidate function not viable: no known conversion from
  'const char *' to 'char **' for 2nd argument
```

## Suggested fix direction (binder)

Add pointer-to-pointer (and more generally: a parameter/return type for which
`make_caster<T>` is ill-formed) to the set of shapes that cause a method to be skipped, the
same way volatile/`&&`/variadic are skipped -- rather than emitting a lambda nanobind cannot
bind.
