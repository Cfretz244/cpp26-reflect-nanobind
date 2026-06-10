# TC-0013 — dependent splice as an `auto`-NTTP argument inside a requires-expression ICEs at parse

- **Status:** CONFIRMED, fix pending (this wave's toolchain batch). Found by the DRIVER while
  implementing BINDER-0020 — not by a corpus run agent.
- **Kind:** parse-time ICE (assertion), `Casting.h:662: dyn_cast on a non-existent value`.
- **Repro:** `corpus/findings/repros/TC-0013/repro.cpp` (11 lines):

  ```cpp
  template <auto> struct probe;
  template <std::meta::info mem>
  bool f() {
      if constexpr (requires { typename probe<([:mem:])>; })  // ICE at parse
          return true;
      return false;
  }
  struct S { static const int K = 7; };
  bool b = f<^^S::K>();
  ```

- **Crash path:** `Parser::ParseRequiresExpression` → `Sema::ActOnTypeRequirement` →
  `CheckTemplateIdType` → `CheckTemplateArgument(NonTypeTemplateParmDecl)` →
  `Sema::DeduceAutoType` → `Expr::ClassifyImpl` → `ClassifyInternal` does
  `dyn_cast<VarDecl>(ValueDecl*)` on a null/absent decl for the dependent splice expression.
  Deducing the `auto` NTTP from a **dependent** splice at parse time classifies an expression
  whose referenced declaration does not exist yet; a FIXED-type NTTP
  (`template <long double> struct probe;` + a cast) takes the non-deducing path and is fine —
  that is the binder's workaround in `reflect_bind_static_member` (BINDER-0020).
- **Root-cause category (tracking issue #308):** E — constant-evaluator/Sema robustness
  (parse-time classification of dependent splice operands).
- **Next (mandatory flow):** clang fix (defer classification/deduction for value-dependent splice
  operands), regression test under
  `llvm-project/libcxx/test/std/experimental/reflection/`, cherry-pick onto bloomberg `p2996`
  tip, file issue + PR, add to #308, record in `repros/TC-0013/UPSTREAM.md`.
