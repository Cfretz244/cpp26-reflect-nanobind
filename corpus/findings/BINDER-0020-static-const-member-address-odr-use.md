# BINDER-0020 — in-class-initialized `static const` members bound by address, ODR-using them → link error

- **Status:** FIXED (binder, `nb_reflect.h`: value path in `reflect_bind_static_member`)
- **Found via:** corpus/runs/concurrentqueue (wave 1; dedup key
  `binder-static-const-member-bound-by-address-odr-use`). `moodycamel::ConcurrentQueue` declares
  seven `static const size_t/uint32_t` config constants with in-class initializers and no
  out-of-line definitions — 14 undefined symbols across the two bound specs.
- **Symptom:** the binding compiles, then fails to link: `Undefined symbols: "...::BLOCK_SIZE",
  referenced from nanobind::detail::reflect_bind_static_member<...>`. The agent worked around it
  by excluding all seven constants via a programmatic `nb::exclude_` marker.
- **Root cause:** `def_ro_static(name, &[:mem:])` takes the member's ADDRESS, which ODR-uses it;
  a non-inline `static const` with only an in-class initializer has no storage.
- **Fix:** const statics whose value is compile-time readable (probed via a FIXED `long double`
  NTTP — in-class initializers are only legal on integral/enum consts and constexpr members, so
  the arithmetic restriction loses nothing; an `auto` NTTP probe ICEs the toolchain, see TC-0013)
  now bind **by value** via `def_prop_ro_static` — the lambda copies `[:mem:]` into a local first,
  because passing it to `cast()` by reference would itself be an ODR-use. Non-constant-readable
  const statics and all mutable statics keep the address path (they require definitions anyway).
- **Verification:** `test45_static_const_by_value` (in-class const int / long long, constexpr
  double, mutable static counter still rw); concurrentqueue stays E — its exclusion workaround is
  now removable as a residual cleanup.
