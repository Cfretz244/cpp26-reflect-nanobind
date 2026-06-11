#!/usr/bin/env python3
"""Per-run gate orchestrator for the binding-test corpus.

Reads <run_dir>/meta.toml, executes the gates via the shell helpers in
corpus/lib, classifies the outcome into the failure taxonomy (A/B/C/D/E/
E-weak), and writes <run_dir>/result.json.

THREE-WAY VALIDATION: when meta.toml carries an [emit] table, a gated run
always executes all three legs and requires them to agree --
  1. the native C++ oracle (ground truth, regenerating tests/expected.json),
  2. the constexpr lane (nb::reflect_ at compile time; Gates 4/5/6),
  3. the emit lane (generated source compiled by the PRODUCTION compiler,
     Apple Clang + system libc++; the same Gates 4/5/6 against the SAME
     expected.json),
then Gate 6b: a module-vs-module API-surface diff (surface_diff.py). The
combined outcome is the worst lane (surface mismatch => D.surface). Runs
without [emit] behave exactly as before (single constexpr lane; the v1
top-level fields are still populated from it).

Usage:  python run_gates.py <run_dir> [--mode constexpr|emit|both]
        (--mode subsets the lanes for debugging; default both-if-[emit])
Exit code: 0 iff every executed lane is E/E-weak and the surface diff passed.
"""
import json
import os
import pathlib
import subprocess
import sys
import time
import tomllib

LIB = pathlib.Path(__file__).resolve().parent
CORPUS = LIB.parent
REPO = CORPUS.parent
TC = REPO / "toolchain"
VENV_PY = REPO / ".venv" / "bin" / "python"

OUTCOME_RANK = {"E": 0, "E-weak": 1, "D": 2, "C": 3, "B": 4, "A": 5}


def sh(args, **kw):
    """Run a command, capture combined output, never raise."""
    return subprocess.run(args, capture_output=True, text=True, **kw)


def run_env(toolchain_dyld=True):
    e = dict(os.environ)
    e.pop("DYLD_LIBRARY_PATH", None)
    if toolchain_dyld:
        # The constexpr lane's modules link the toolchain libc++. The emit
        # lane must NOT see this: dyld overrides by LEAF name, so it would
        # hijack the system libc++.1.dylib inside a prod-built module.
        e["DYLD_LIBRARY_PATH"] = str(TC / "lib")
    return e


def first_lines(text, n=5):
    out, seen = [], set()
    for ln in text.splitlines():
        ln = ln.strip()
        if ln and ln not in seen:
            seen.add(ln)
            out.append(ln)
        if len(out) >= n:
            break
    return out


def detect_toolchain_bug(diag_text):
    """Heuristic: does this diagnostic look like a toolchain bug (vs library/binder)?

    Applies to REFLECTION-toolchain diagnostics only (the constexpr lane, and
    the emit lane's stage 1); Apple Clang output must never run through this.
    """
    t = diag_text.lower()
    if "__type_descriptor_t" in diag_text:
        return {"status": "suspected", "kind": "unresolved-weak-symbol",
                "signature": "__ZnamSt19__type_descriptor_t",
                "finding": "TC-0001-typed-operator-new-at-O0"}
    if "invalid slocoffset" in t or "getfileidloaded" in t:
        return {"status": "suspected", "kind": "ice",
                "signature": "SourceManager getFileIDLoaded: Invalid SLocOffset",
                "finding": "TC-0002-sloc-ice-heavy-reflection"}
    if "maximum step limit" in t:
        # Not itself a compiler bug, but on this toolchain raising the limit to finish ICEs
        # the compiler (TC-0002). Flag so the heavy-type scale wall is tracked.
        return {"status": "suspected", "kind": "constexpr-step-limit",
                "signature": "constexpr evaluation hit maximum step limit",
                "finding": "TC-0002-sloc-ice-heavy-reflection"}
    # A *static* assertion is a deliberate source-level diagnostic (e.g. the
    # binder's BINDER-0014 completeness gate naming a missing caster header),
    # not a compiler self-assert -- drop it before the generic "assertion
    # failed" ICE match below (false-positived during the unordered_dense run).
    t = t.replace("static assertion failed", "").replace("static_assert", "")
    for marker in ("unreachable", "internal compiler error", "please submit a bug report",
                   "assertion failed", "mangling a placeholder type", "stack dump"):
        if marker in t:
            return {"status": "suspected", "kind": "compiler-crash",
                    "signature": marker, "finding": ""}
    return {"status": "none"}


def lane_record():
    return {
        "outcome": None,
        "furthest_gate": 1,
        "gate_results": {},
        "failure": {"code": None, "first_diagnostics": [], "signal": None},
        "metrics": {},
        "oracle": {"layers_used": [], "ground_truth_source": ""},
    }


def strip_reflection_flags(flags):
    """Stage-2 (production compile) extra_cflags: drop reflection-only flags,
    keep -D defines (the generated TU re-preprocesses the library headers) and
    warning tweaks."""
    return [f for f in flags
            if not f.startswith("-fconstexpr-steps")
            and not f.startswith("-freflection")]


def run_lane(lane, run, meta, incflags, extra_cflags, extra_sources,
             extra_libs, mod, strategy, classify_tc_bug):
    """Gates 4/5/6 for one lane. Returns (lane_dict, build_dir)."""
    L = lane_record()
    is_emit = lane == "emit"
    build_dir = run / "binding" / ("build-emit" if is_emit else "build")
    emit_meta = meta.get("emit", {})

    build_env = dict(os.environ)
    if extra_sources:
        build_env["NB_EXTRA_SOURCES"] = " ".join(extra_sources)
    if is_emit:
        # Mechanical prod link line: the prod archive variants install to
        # <prefix>-prod (build_cmake_lib.sh --prod / build_abseil.sh --prod).
        if extra_libs:
            build_env["NB_EXTRA_LIBS_PROD"] = extra_libs.replace(
                "-install/lib", "-install-prod/lib").replace(
                "abseil-install/lib", "abseil-install-prod/lib")
        build_env["NB_GEN_CFLAGS"] = " ".join(extra_cflags)
        build_env["NB_PROD_CFLAGS"] = " ".join(
            strip_reflection_flags(emit_meta.get("extra_cflags", extra_cflags)))
        if emit_meta.get("std"):
            build_env["NB_PROD_STD"] = emit_meta["std"]
    elif extra_libs:
        build_env["NB_EXTRA_LIBS"] = extra_libs

    # ---- Gate 4: compile ----
    t0 = time.time()
    if is_emit:
        b = sh(["bash", str(LIB / "build_module_emit.sh"),
                str(run / "binding" / "gen_emit.cpp"), mod, str(build_dir),
                *incflags], env=build_env)
    elif strategy == "two_stage":
        b = sh(["bash", str(LIB / "build_module_codegen.sh"),
                str(run / "binding" / "gen.cpp"), str(run / "binding" / "binding.cpp"),
                mod, str(build_dir), *incflags, *extra_cflags], env=build_env)
    else:
        b = sh(["bash", str(LIB / "build_module.sh"),
                str(run / "binding" / "binding.cpp"), mod, str(build_dir),
                *incflags, *extra_cflags], env=build_env)
    L["metrics"]["binding_compile_seconds"] = round(time.time() - t0, 2)
    if b.returncode != 0:
        L["gate_results"]["4_compile"] = "fail"
        L["outcome"] = "B"
        stage = next((l.split("=", 1)[1] for l in b.stderr.splitlines()
                      if l.startswith("BUILD_FAIL_STAGE=")), "module")
        L["failure"]["code"] = f"B.{stage}"
        L["failure"]["first_diagnostics"] = first_lines(b.stderr + b.stdout)
        # The emit lane's stage-2 diagnostics come from Apple Clang; only its
        # stage 1 (the reflection-built generator) feeds the bug classifier.
        if not is_emit or stage in ("emit_gen_compile", "emit_gen_run"):
            classify_tc_bug(b.stderr + b.stdout)
        return L, build_dir
    L["gate_results"]["4_compile"] = "pass"
    L["furthest_gate"] = 4
    if is_emit:
        gen = build_dir / "binding.gen.cpp"
        if gen.exists():
            text = gen.read_text()
            L["metrics"]["generated_tu_lines"] = text.count("\n")
            L["metrics"]["generated_tu_bytes"] = len(text)

    # ---- Gate 5: import smoke ----
    imp = sh([str(VENV_PY), "-c", f"import {mod}"],
             env={**run_env(toolchain_dyld=not is_emit),
                  "PYTHONPATH": str(build_dir)})
    if imp.returncode != 0:
        L["gate_results"]["5_import"] = "fail"
        L["outcome"] = "C"
        sig = "SIGSEGV" if imp.returncode in (-11, 139) else ("Import" if "Error" in imp.stderr else str(imp.returncode))
        L["failure"]["code"] = f"C.{sig.lower()}"
        L["failure"]["signal"] = sig
        L["failure"]["first_diagnostics"] = first_lines(imp.stderr)
        return L, build_dir
    L["gate_results"]["5_import"] = "pass"
    L["furthest_gate"] = 5

    # ---- Gate 6: correctness (same suite, same expected.json, per-lane cache) ----
    test_py = run / "tests" / "test_bindings.py"
    if not test_py.exists():
        L["outcome"] = "E-weak"
        L["gate_results"]["6_correct"] = "skip"
        L["furthest_gate"] = 6
        return L, build_dir
    cache = run / "tests" / "build" / (".pytest_cache_emit" if is_emit
                                       else ".pytest_cache")
    t = sh([str(VENV_PY), "-m", "pytest", str(test_py), "-q",
            "-o", f"cache_dir={cache}"],
           env={**run_env(toolchain_dyld=not is_emit),
                "PYTHONPATH": str(build_dir)})
    L["gate_results"]["6_correct"] = "pass" if t.returncode == 0 else "fail"
    tail = t.stdout.strip().splitlines()[-1] if t.stdout.strip() else ""
    L["oracle"]["pytest_summary"] = tail
    L["furthest_gate"] = 6
    if t.returncode != 0:
        L["outcome"] = "D"
        L["failure"]["code"] = "D.value"
        L["failure"]["first_diagnostics"] = first_lines(t.stdout, 8)
        return L, build_dir
    L["outcome"] = None  # E vs E-weak decided by the caller (oracle knowledge)
    return L, build_dir


def main(run_dir, mode=None):
    run = pathlib.Path(run_dir).resolve()
    meta = tomllib.loads((run / "meta.toml").read_text())
    inc = run / meta["include_root"] if meta.get("include_root") else None
    incflags = ["-I", str(inc)] if inc else []
    # Per-run extra compile flags (e.g. a raised -fconstexpr-steps for heavy
    # reflection like nlohmann/json's basic_json). Recorded in meta.toml for
    # reproducibility; applied to the reflection compiles (Gate 4 stage 1).
    extra_cflags = meta.get("extra_cflags", [])
    # Per-run extra library source files for non-header-only libs: compiled and
    # linked into the module (constexpr lane: by the toolchain via
    # build_module.sh; emit lane: by the PRODUCTION compiler via
    # build_module_emit.sh).
    extra_sources = [str((run / s).resolve()) for s in meta.get("extra_sources", [])]
    # Non-header-only via a prebuilt static lib. Two forms:
    #   - generic: `extra_libs = "-L {repo}/build/<slug>-install/lib -l<slug>_merged"`
    #   - legacy: `link_abseil = true` links the merged Abseil archive.
    # The emit lane mechanically rewrites the prefix to the -prod variant.
    abseil_prefix = os.environ.get("NB_ABSEIL_PREFIX", str(REPO / "build" / "abseil-install"))
    extra_libs = meta.get("extra_libs", "").replace("{repo}", str(REPO))
    if meta.get("link_abseil", False):
        # CoreFoundation: absl's cctz time-zone lookup (transitively in the merged archive)
        # references it on macOS.
        extra_libs = (extra_libs + " " if extra_libs else "") + \
            f"-L {abseil_prefix}/lib -labsl_merged -framework CoreFoundation"
    mod = meta["module_name"]
    strategy = meta.get("strategy", "single_stage")

    has_emit = "emit" in meta and meta["emit"].get("enabled", True)
    if mode is None:
        lanes_to_run = ["constexpr", "emit"] if has_emit else ["constexpr"]
    elif mode == "both":
        lanes_to_run = ["constexpr", "emit"]
    else:
        lanes_to_run = [mode]

    r = {
        "schema_version": 2,
        "slug": meta["slug"],
        "url": meta.get("url", ""),
        "pinned_commit": meta.get("pin", ""),
        "tier": meta.get("tier"),
        "header_only": meta.get("header_only", True),
        "toolchain_commit": sh(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD:llvm-project"]).stdout.strip() or "unknown",
        "binder_commit": sh(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD:nanobind"]).stdout.strip() or "unknown",
        "prod_compiler": "",
        "outcome": None,
        "furthest_gate": 0,
        "gate_results": {"3_strategy": strategy},
        "lanes": {},
        "surface_diff": {"status": "skip", "mismatches": []},
        "subset": {"reflect_args": meta.get("reflect_args", []),
                   "rationale": meta.get("subset_rationale", ""),
                   "skipped_features": meta.get("skipped_features", [])},
        "oracle": {"layers_used": [], "ground_truth_source": ""},
        "failure": {"code": None, "first_diagnostics": [], "signal": None},
        "toolchain_bug": {"status": "none"},
        "metrics": {},
        "artifacts": {},
        "notes": "",
    }
    if "emit" in lanes_to_run:
        v = sh([os.environ.get("PROD_CXX", "/usr/bin/clang++"), "--version"])
        r["prod_compiler"] = v.stdout.splitlines()[0] if v.stdout else "unknown"

    def classify_tc_bug(text):
        if r["toolchain_bug"].get("status", "none") == "none":
            r["toolchain_bug"] = detect_toolchain_bug(text)

    # ---- Gate 1: probe (shared) ----
    probe = run / "probe" / "probe.cpp"
    if not probe.exists():
        probe.parent.mkdir(parents=True, exist_ok=True)
        probe.write_text("".join(f'#include "{h}"\n' for h in meta.get("main_headers", [])))
    t0 = time.time()
    p = sh(["bash", str(LIB / "probe.sh"), str(probe), *incflags])
    r["metrics"]["probe_seconds"] = round(time.time() - t0, 2)
    if p.returncode != 0:
        r["gate_results"]["1_probe"] = "fail"
        r["outcome"] = "A"
        r["failure"]["code"] = "A.compile"
        r["failure"]["first_diagnostics"] = first_lines(p.stderr)
        r["toolchain_bug"] = detect_toolchain_bug(p.stderr)
        return finish(run, r)
    r["gate_results"]["1_probe"] = "pass"
    r["furthest_gate"] = 1

    # ---- The oracle: leg 1 of the three-way validation, built ONCE (same
    # ---- ground truth on both module legs).
    oracle = run / "tests" / "oracle_native.cpp"
    has_diff = oracle.exists()
    if has_diff:
        build_env = dict(os.environ)
        if extra_sources:
            build_env["NB_EXTRA_SOURCES"] = " ".join(extra_sources)
        if extra_libs:
            build_env["NB_EXTRA_LIBS"] = extra_libs
        ob = sh(["bash", str(LIB / "build_native.sh"), str(oracle),
                 str(run / "tests" / "build" / "oracle"), *incflags], env=build_env)
        if ob.returncode == 0:
            res = sh([str(run / "tests" / "build" / "oracle")], env=run_env())
            if res.returncode == 0 and res.stdout.strip():
                (run / "tests" / "expected.json").write_text(res.stdout)
                r["oracle"]["layers_used"].append("L1_differential")
                r["oracle"]["ground_truth_source"] = "native C++ oracle (oracle_native.cpp)"
            else:
                r["notes"] += "oracle ran but produced no output; "
                has_diff = False
        else:
            r["notes"] += "native oracle failed to build; "
            has_diff = False

    # ---- Lanes: Gates 4/5/6 per backend ----
    has_tests = (run / "tests" / "test_bindings.py").exists()
    for lane in lanes_to_run:
        L, _bdir = run_lane(lane, run, meta, incflags, extra_cflags,
                            extra_sources, extra_libs, mod, strategy,
                            classify_tc_bug)
        if L["outcome"] is None:  # passed Gate 6
            L["outcome"] = "E" if (has_diff and has_tests) else "E-weak"
        if not has_tests and L["gate_results"].get("6_correct") == "skip":
            r["notes"] += f"{lane}: no test_bindings.py (import-only); "
        if has_diff and L["furthest_gate"] >= 6:
            L["oracle"]["layers_used"] = ["L1_differential"]
            L["oracle"]["ground_truth_source"] = "native C++ oracle (oracle_native.cpp)"
        r["lanes"][lane] = L

    # ---- Gate 6b: surface diff (only when both lanes imported) ----
    both_imported = (len(lanes_to_run) == 2 and all(
        r["lanes"].get(l, {}).get("gate_results", {}).get("5_import") == "pass"
        for l in ("constexpr", "emit")))
    if both_imported:
        ignores = meta.get("emit", {}).get("surface_diff_ignore", [])
        args = [str(VENV_PY), str(LIB / "surface_diff.py"), "compare",
                str(run), mod]
        for pat in ignores:
            args += ["--ignore", pat]
        sd = sh(args)
        lines = [l for l in sd.stdout.splitlines()
                 if l and not l.startswith("surface_diff:")]
        r["surface_diff"] = {
            "status": "pass" if sd.returncode == 0 else "fail",
            "mismatches": lines[:50],
        }
        tailln = next((l for l in sd.stdout.splitlines()
                       if l.startswith("surface_diff:")), "")
        if tailln:
            r["notes"] += tailln + "; "

    # ---- Combined outcome: worst lane; surface mismatch is a D ----
    worst = None
    for lane in lanes_to_run:
        o = r["lanes"][lane]["outcome"]
        if worst is None or OUTCOME_RANK[o] > OUTCOME_RANK[worst]:
            worst = o
    if r["surface_diff"]["status"] == "fail" and OUTCOME_RANK.get(worst, 0) < OUTCOME_RANK["D"]:
        worst = "D"
        r["failure"]["code"] = "D.surface"
    r["outcome"] = worst

    # ---- Legacy top-level mirror (v1 tool compatibility): constexpr lane ----
    cx = r["lanes"].get("constexpr")
    if cx:
        r["gate_results"].update(cx["gate_results"])
        r["furthest_gate"] = cx["furthest_gate"]
        if r["failure"]["code"] is None and cx["failure"]["code"]:
            r["failure"] = cx["failure"]
        r["oracle"]["pytest_summary"] = cx["oracle"].get("pytest_summary", "")
        r["metrics"].update({f"constexpr_{k}": v for k, v in cx["metrics"].items()})
    em = r["lanes"].get("emit")
    if em:
        r["metrics"].update({f"emit_{k}": v for k, v in em["metrics"].items()})
        if r["failure"]["code"] is None and em["failure"]["code"]:
            r["failure"] = em["failure"]

    return finish(run, r)


def finish(run, r):
    (run / "result.json").write_text(json.dumps(r, indent=2) + "\n")
    lane_bits = " ".join(
        f"{l}={d['outcome']}" for l, d in r.get("lanes", {}).items())
    sd = r.get("surface_diff", {}).get("status", "skip")
    print(f"[{r['slug']}] outcome={r['outcome']} {lane_bits} surface={sd} "
          f"{r['failure']['code'] or ''}".strip())
    if r["toolchain_bug"].get("status", "none") != "none":
        print(f"  toolchain_bug: {r['toolchain_bug']['status']} "
              f"({r['toolchain_bug'].get('kind')}) {r['toolchain_bug'].get('finding','')}")
    ok = r["outcome"] in ("E", "E-weak") and sd != "fail"
    return 0 if ok else 1


if __name__ == "__main__":
    args = sys.argv[1:]
    mode = None
    if "--mode" in args:
        i = args.index("--mode")
        mode = args[i + 1]
        args = args[:i] + args[i + 2:]
        if mode not in ("constexpr", "emit", "both"):
            print("--mode must be constexpr|emit|both", file=sys.stderr)
            sys.exit(2)
    if len(args) != 1:
        print("usage: run_gates.py <run_dir> [--mode constexpr|emit|both]", file=sys.stderr)
        sys.exit(2)
    sys.exit(main(args[0], mode))
