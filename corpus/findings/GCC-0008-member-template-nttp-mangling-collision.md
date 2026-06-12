# GCC-0008 — same-named member-template reflection NTTPs mangle identically

- dedup_key: `gcc8-member-template-nttp-mangling`
- compiler: g++ (GCC) 16.1.0 aarch64-linux-gnu (`gcc:16` docker image)
- probe: `gcc16-proveout/probes/xfail_gcc8_member_template_nttp_mangling.cpp`
- status: workaround landed in the binder (`member_tmpl_mangle_hint`);
  upstream report planned (Phase 4; gcc.gnu.org bugzilla, component c++)
- found by: corpus run `unordered_dense`

## Symptom

```
/tmp/cc*.s: Assembler messages:
Error: symbol `_ZN...reflect_bind_member_templateI...tableI...ELDmftSN_2atE...' is already defined
```

A function template instantiated once per same-named sibling member template
— the binder's `reflect_bind_member_template<T, tmpl>` over ankerl
unordered_dense `table`'s const/non-const heterogeneous `at(K)` pair — emits
TWO definitions under ONE assembly symbol: GCC mangles a TEMPLATE-kind
reflection NTTP by NAME only (`LDmftSN_2atE` encodes just "at"), so the
sibling instantiations collide.

This is stock GCC's instance of the same design gap the clang-p2996 fork had
(its TC-0004/TC-0009, fixed there by folding an ODR hash of the template
head + pattern type into the reflection-NTTP mangling). Depending on linkage
the failure mode is an assembler hard error (seen here) or — worse — silent
linkonce folding where one body serves both call sites (the clang fork's
TC-0004 field shape).

## Repro

The probe is ~60 lines, self-contained:

```
g++ -std=c++26 -freflection xfail_gcc8_member_template_nttp_mangling.cpp
```

Expected: builds, runs, exits 0. Actual: `Error: symbol
'_Z8dispatchI5TableLDmftS0_2atEEvRT_Ri' is already defined`.

## Binder workaround (landed)

`member_tmpl_mangle_hint(tmpl)` — the default-instantiation SPEC (a
Declaration-kind reflection, mangled by signature) folded into
`reflect_bind_member_template`'s template-id as a defaulted NTTP, keeping
sibling instantiations distinct on both compilers. A
non-default-instantiable sibling contributes `^^void`; it can only collide
with another uninstantiable same-named sibling, whose instantiations are
both empty (route == skip). Unit regression: `HetMap::hat` const/non-const
pair in the shared fixture. Both suites 129/129 on both compilers.
