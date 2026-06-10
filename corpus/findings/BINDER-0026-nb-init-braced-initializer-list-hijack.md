# BINDER-0026 — nb::init's braced construction let initializer_list ctors hijack reflected constructors

- **Status:** FIXED (binder: `reflect_init`/`reflect_construct_at` — parens construction
  selecting exactly the reflected overload, braces only as the aggregate fallback; used by
  all reflected-ctor paths incl. BINDER-0013 copy-init).
- **Found via:** corpus/runs/immer (wave 2; dedup key
  `binder-init-braced-init-hijacked-by-initializer-list-ctor`): `immer::vector<int>`'s
  `(size_type, T)` fill ctor brace-initialized to `initializer_list<int>` and the
  size_type NARROWED — hard compile error; in non-narrowing shapes braces silently run the
  WRONG constructor.
- **Verification:** `test51_ctor_parens_no_initializer_list_hijack` (FillVec(3, 9) is a
  fill: size 3, sum 27). The immer run's per-member ctor exclusion is now removable as a
  residual.
