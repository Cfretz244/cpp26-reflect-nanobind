# GCC-0003 upstream record — splice of an info NTTP escalates the lambda to consteval

- status: **PATCH READY** — upgraded from "divergence question" to a real
  bug with a one-hunk fix (not yet filed — filing is the user's call)
- repro: `gcc16-proveout/probes/xfail_gcc3_splice_lambda_consteval.cpp`
- patch: `0001-cxx-reflection-splice-operands-do-not-escalate.patch`
  (commit `400daa86161` on the devenv `proveout-fixes` branch, base master
  `7ce3a7b1beb`)
- affected: GCC 16.1 (release) and trunk 17.0 @ 7ce3a7b1beb (verified both)

## Root cause (debugger-verified)

A lambda inside a function template whose body splices an enclosing
`std::meta::info` NTTP — `self.[:fn:]()` — cannot be converted to a plain
function pointer: GCC makes the closure's `operator()`/`_FUN` consteval, so
the conversion errors with "immediate evaluation returns address of
immediate function".

Mechanism: `check_out_of_consteval_use_r` (gcc/cp/reflect.cc) escalates an
immediate-escalating function to consteval when its body contains a
consteval-only expression. The walker descends into the operand of the
dependent splice (`SPLICE_EXPR`) and finds the `TEMPLATE_PARM_INDEX` of
`fn`, whose type `std::meta::info` is consteval-only (gdb: the offending
tree at the `promote_function_to_consteval` call for the closure's
`operator()` is exactly that TEMPLATE_PARM_INDEX, at template-definition
time).

Conformance argument: the constant-expression of a splice-specifier is
manifestly constant-evaluated — an immediate function context — so a
consteval-only entity used inside it never escapes to runtime and is not an
immediate-escalating expression ([expr.const]). The walker already applies
exactly this exemption to `if constexpr` conditions and to arguments of
immediate invocations; splice operands belong in the same class. clang-p2996
accepts the probe.

## The fix

One hunk in `check_out_of_consteval_use_r`: do not walk into `SPLICE_EXPR`
operands. Non-dependent splices are resolved at parse and never appear in
the walk; after instantiation a dependent splice is resolved to the
designated entity, which the walker still sees normally — so genuine
consteval-only leaks (e.g. a splice RESULT of info type flowing to runtime)
are still diagnosed on the resolved trees.

## Verification (devenv, trunk @ 7ce3a7b1beb + all three proveout patches)

- The probe compiles, runs, prints `via fp: 1`, exit 0.
- Reflection + constexpr testsuite slices: 3939 passes / 0 unexpected
  failures (joint run; see GCC-0008/UPSTREAM.md).
- New regression test `g++.dg/reflect/splice-no-escalation1.C` (dg-do run).
- The binder's portable hoist workaround (`reflect_bind_conversion` hoisting
  `&[:fn:]` to a constexpr pointer-to-member) remains valid and unneeded
  under the patch.

## Bugzilla material (component c++)

- Summary: `[C++26] -freflection: lambda splicing an enclosing
  std::meta::info NTTP is wrongly escalated to consteval (cannot decay to
  function pointer)`
- Description: root cause + conformance argument above; expected = lambda
  stays non-consteval (the splice operand is manifestly constant-evaluated);
  actual = "immediate evaluation returns address of immediate function";
  clang-p2996 cross-check accepts.
- Attach the probe. CC the reflection implementers (as in GCC-0008).
