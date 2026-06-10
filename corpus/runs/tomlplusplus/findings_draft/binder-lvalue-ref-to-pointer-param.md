dedup_key: binder-method-lvalue-ref-to-pointer-out-param-hard-error

# Binder: a method with a non-const lvalue-reference-to-pointer parameter hard-errors instead of being skipped

## Layer
BINDER (nb_reflect.h)

## Summary
`reflect_bind_method` / `reflect_method_binder` will attempt to bind a method whose
parameter is a non-const lvalue reference to a pointer (an out-parameter, `T*&`). The
generated forwarding lambda `[](Self&, ..., T*& out) -> Ret { ... }` is then handed to
nanobind's `cpp_function_def` / `func_create`, which has no Python representation for a
`T*&` argument and fails to form the callable -- a hard compile error
(`nb_func.h:292: no matching function for call to object of type '... (lambda ...)'`),
not a graceful skip.

The binder already skips several unbindable shapes (volatile, rvalue-ref-qualified `&&`,
C-variadic, move-only-by-value params per BINDER-0010), but a non-const lvalue-reference
parameter whose referent has no Python representation (here `node*&`) is not among them,
so it reaches the method binder and breaks the whole translation unit.

## Smallest trigger
toml++ v3.4.0: `toml::node` (and `table`/`array`/`value<T>`) declare
```cpp
virtual bool is_homogeneous(node_type ntype, node*& first_nonmatch) noexcept;
virtual bool is_homogeneous(node_type ntype, const node*& first_nonmatch) const noexcept;
```
Reflecting `^^toml::node` (or any node-derived class) tries to bind these and aborts the
build. A minimal standalone repro:
```cpp
struct Out;
struct Thing { bool peek(Out*& out) noexcept; };   // Out*& out-param
// nb::reflect_<^^Thing>(m);  // -> nb_func.h hard error, not a skip
```

## First diagnostics
```
nanobind/include/nanobind/nb_func.h:292:24: error: no matching function for call to
  object of type 'const std::remove_reference_t<(lambda at nb_reflect.h:397:1)>'
  ... requested from reflect_method_binder<toml::node, ^^(declaration),
      bool (toml::node_type, toml::node *&) noexcept>::bind<...>
  ... reflect_bind_member_function<toml::node, ...>
```

## Suggested shape of fix
Extend the `reflect_bind_method` skip guard (nb_reflect.h ~413) to also skip any function
whose parameter list contains a non-const lvalue reference to a type with no value/caster
representation (at minimum, a reference-to-pointer). Mirrors the existing
`has_move_only_by_value_param` / volatile / `&&` graceful-skip predicates.

## Workaround attempted (and why it FAILED)
I collected the exact `is_homogeneous(node_type, node*&)` / `(const node*&)` overload
reflections via a consteval `members_of(^^toml::node, unchecked())` filter and fed them to
`nb::exclude_<bad...>` (whose `fn_mentions_excluded` checks `info_span_contains(ex, fn)`
against the function's OWN info, line 975). Within a single TU the collected infos compare
`==` to the same members re-enumerated (verified: 2 matched / 2 total). Yet the binder STILL
binds node's `is_homogeneous(node_type, node*&)` and hard-errors. So either the bind loop's
per-member `fn` reflection is not `==` to the one `members_of` yields at the call site (an
info-identity divergence across the bind path), or the per-member exclusion gate is not
consulted on this path. Net: a single function overload cannot be excluded from the call
site here at all, which is a SECOND, distinct gap (member-granularity exclusion appears
ineffective for these overloads) layered on top of the primary `T*&`-param hard-error.

Minimal repro of the failed exclusion (standalone, no corpus harness):
`nb::reflect_<^^toml::node, ^^nb::exclude_<...the two node*& overload infos...>>(m)` still
errors at `nb_func.h:292`.

## Consequence for the run
The node class hierarchy (node/table/array/value<T>, all of which carry these out-param
overloads) cannot be bound head-on under the current binder. The run falls back to binding
the value-type structs + parse surface head-on and routing all tree navigation through
fixture functions that return plain Python types (see meta.toml subset_rationale /
skipped_features).
