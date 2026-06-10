// TC-0002 — Sema use-after-free: ExprEvalContexts reallocates under a held record
// reference during reentrant consteval evaluation.
// Standalone repro (no nanobind). See corpus/findings/TC-0002-sloc-ice-heavy-reflection.md.
//
// Mechanism: Sema::PopExpressionEvaluationContext binds
//   ExpressionEvaluationContextRecord &Rec = ExprEvalContexts.back();
// then calls HandleImmediateInvocations(*this, Rec), which constant-evaluates the
// pending immediate (consteval) invocations of the popped context. Reflection
// metafunctions evaluated there REENTER Sema: substitute() instantiates the named
// specialization (and, for an undeduced signature, its body —
// MetaActions::Substitute), and instantiation pushes/pops expression evaluation
// contexts. Once ExprEvalContexts (a SmallVector<..., 8>) grows past its current
// capacity it reallocates, leaving Rec — and every other reference into the old
// storage — dangling. The field crash (binding nlohmann::json) was a
// non-deterministic SIGSEGV on ASCII-looking garbage in PopExpressionEvaluationContext.
//
// This file makes the dangle structural, not luck-based:
//   - start(n) is a consteval call in a RUNTIME function body, so it is deferred as
//     an immediate-invocation candidate and evaluated inside
//     HandleImmediateInvocations — while Rec is held.
//   - Its evaluation calls substitute(^^chain, {n}) with a value that exists only
//     at evaluation time, so nothing is instantiated during parsing: the entire
//     instantiation cascade happens INSIDE the held-reference window.
//   - chain<N> has a DEDUCED return type, so substitute() must instantiate its
//     body; the body recurses through a dependent splice-of-substitute, which is
//     resolved (and its operand evaluated) per instantiation. Depth 64 nests
//     dozens of evaluation contexts: 8 -> 16 -> 32 -> 64 -> ... — several
//     heap-to-heap reallocations while the outer Rec is live.
//
// On a compiler WITHOUT the fix this is a use-after-free in
// PopExpressionEvaluationContext's tail (crash/garbage is heap-layout dependent;
// an address-comparison probe — see the finding — fires deterministically).
// With the fix (re-acquire the record after reentrant evaluation) this compiles
// and runs cleanly; the program exits 0.
//
// Build & run (from the umbrella repo root):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
//     -L $TC/lib -Wl,-rpath,$TC/lib \
//     corpus/findings/repros/TC-0002/repro.cpp -o corpus/findings/repros/TC-0002/repro
//   DYLD_LIBRARY_PATH=$TC/lib ./corpus/findings/repros/TC-0002/repro && echo OK

#include <meta>

#include <cstdio>
#include <vector>

template <int N>
consteval auto chain() {  // deduced return type => substitute() instantiates the body
  if constexpr (N <= 0) {
    return 0;
  } else {
    // Dependent splice: resolved during THIS specialization's instantiation,
    // evaluating substitute(^^chain, {N-1}) then — recursing the instantiation
    // one level deeper while every outer instantiation frame is still pushed.
    return [:std::meta::substitute(
        ^^chain,
        std::vector<std::meta::info>{std::meta::reflect_constant(N - 1)}):]() + 1;
  }
}

consteval int start(int n) {
  // `n` exists only at constant-evaluation time, so this substitute() — and the
  // whole chain<n> instantiation cascade behind it — runs while
  // HandleImmediateInvocations is evaluating THIS call.
  return std::meta::extract<int (*)()>(std::meta::substitute(
      ^^chain, std::vector<std::meta::info>{std::meta::reflect_constant(n)}))();
}

int main() {
  // Runtime context: start(64) is deferred as an immediate-invocation candidate
  // and evaluated at PopExpressionEvaluationContext with `Rec` held.
  int x = start(64);
  std::printf("chain depth evaluated: %d\n", x);
  return x == 64 ? 0 : 1;
}
