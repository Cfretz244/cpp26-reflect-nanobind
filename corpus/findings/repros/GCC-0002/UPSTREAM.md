# GCC-2 upstream record — constexpr local as expansion range: GCC IS CONFORMING

- status: **RECLASSIFIED — NOT A GCC BUG; do not file** (2026-06-12).
  The divergence is clang-p2996 ACCEPTING ill-formed code.
- repro: `gcc16-proveout/probes/xfail_gcc2_constexpr_local_range.cpp`
  (still expected-fail on GCC by design — it is ill-formed)

## The conformance analysis

[stmt.expand]/5.2 (P1306R5 as adopted; verified against the current draft,
eel.is/c++draft/stmt.expand) makes an iterating expansion statement
equivalent to:

```
{
  init-statement
  constexpr_opt decltype(auto) range = ( expansion-initializer );
  constexpr_opt auto begin = begin-expr;
  S_0 ... S_{N-1}
}
```

where `constexpr` is present exactly when the for-range-declaration has it
(the probe's does: `template for (constexpr auto i : indices)`).
`decltype(auto)` applied to the PARENTHESIZED lvalue `(indices)` deduces a
REFERENCE type, so the synthesized range variable is a **constexpr
reference**, whose initializer must be a constant expression — i.e. the
address of `indices` must be an address constant. A non-static local
constexpr variable has no constant address, so the program is ill-formed.
GCC 16.1's diagnostic is exactly on point, fix-it included:

```
error: 'indices' is not a constant expression
note: reference to 'indices' is not a constant expression
note: address of non-static constexpr variable 'indices' may differ on each
      invocation of the enclosing function; add 'static' to give it a
      constant address
```

Verified in the devenv (trunk @ 7ce3a7b1beb): adding `static` to the
probe's local makes it compile, run, and print `sum=6`. An inline
(prvalue) expansion-initializer also works because the materialized
temporary is constant-initializable in the synthesized declaration; a
variable template (the binder's `emit_indices_v`) works for the same
reason — static storage.

Jakub Jelinek's P1306R5 implementation commit (458773ac7bc) discusses this
exact corner ("I guess my preference would be dropping those static
keywords from [stmt.expand]" — the synthesized variables' storage/wording
was under CWG discussion, cf. CWG 3043–3048), but under the adopted +
current-draft wording the rejection stands regardless of the
static-keyword question, because the constexpr REFERENCE still needs an
address constant.

## Consequences

- Do NOT file on gcc.gnu.org. (Optionally reportable against clang-p2996
  as accepts-invalid; low value while its expansion-statement
  implementation tracks an earlier draft.)
- The binder's `emit_indices_v` variable-template hoist
  (nb_reflect_emit.h) is the CORRECT portable spelling, not a workaround
  for a GCC defect — keep it; the probe header now says so.
- If an EWG/CWG paper relaxes [stmt.expand] (Jakub's preference hints the
  door is open), re-test the probe and revisit.
