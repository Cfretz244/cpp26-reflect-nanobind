# BINDER-0015 — unbindable parameter/return shapes hard-error the build instead of skipping

- **Status:** FIXED (binder, `nb_reflect.h`: `is_unbindable_shape` / `has_unbindable_signature`)
- **Found via:** corpus/runs/cli11 (`char** ensure_utf8(char**)`, argv-style `parse(int, const char* const*)`)
  and corpus/runs/tomlplusplus (`virtual bool is_homogeneous(node_type, node*&)` on every node class) —
  wave 1 parallel dispatch; two agents, one root cause (dedup keys
  `binder-ptr-to-ptr-param-no-graceful-skip`, `binder-method-lvalue-ref-to-pointer-out-param-hard-error`).
- **Symptom:** reflecting any class with a method whose parameter (or return) is a pointer-to-pointer,
  a pointer-to-function, or a non-const lvalue reference to a pointer (`T*&` out-param) aborted the
  whole TU: the method binder synthesized a forwarding lambda nanobind's `func_create` cannot match
  (`nb_func.h:292: no matching function for call to object of type '... lambda ...'`).
- **Root cause:** the graceful-skip set (volatile / `&&` / C-variadic / by-value move-only BINDER-0010)
  did not cover parameter/return TYPES that have no caster and no Python representation at all.
- **Fix:** `is_unbindable_shape` (dealiased; ref-to-non-const-pointer, pointer-to-pointer,
  pointer-to-function) + `has_unbindable_signature` gate on every binding path — methods, ctors,
  free functions, free operators, member-template default instantiations, entity proxies — and
  mirrored in the STL caster-collection walk (the BINDER-0011 rule: a skipped function must not
  demand casters).
- **Verification:** `test42_unbindable_shapes_skip` (Gadget: `mangle`/`peek`/`on_event` absent, clean
  surface binds); cli11 re-gated **D→E**; tomlplusplus stays E (its node-tree subset can now be
  revisited as a residual). Full suite 58/58.
