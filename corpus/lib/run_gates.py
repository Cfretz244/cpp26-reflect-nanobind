#!/usr/bin/env python3
"""Per-run gate orchestrator for the binding-test corpus.

Reads <run_dir>/meta.toml, executes Gates 1/4/5/6 via the shell helpers in corpus/lib,
classifies the outcome into the failure taxonomy (A/B/C/D/E/E-weak), and writes
<run_dir>/result.json. This is the reusable machinery every per-repo agent invokes.

Usage:  python run_gates.py <run_dir>
Exit code mirrors success: 0 if outcome in {E, E-weak}, else 1.
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


def sh(args, **kw):
    """Run a command, capture combined output, never raise."""
    return subprocess.run(args, capture_output=True, text=True, **kw)


def run_env():
    e = dict(os.environ)
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
    """Heuristic: does this diagnostic look like a toolchain bug (vs library/binder)?"""
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
    for marker in ("unreachable", "internal compiler error", "please submit a bug report",
                   "assertion failed", "mangling a placeholder type", "stack dump"):
        if marker in t:
            return {"status": "suspected", "kind": "compiler-crash",
                    "signature": marker, "finding": ""}
    return {"status": "none"}


def main(run_dir):
    run = pathlib.Path(run_dir).resolve()
    meta = tomllib.loads((run / "meta.toml").read_text())
    inc = run / meta["include_root"] if meta.get("include_root") else None
    incflags = ["-I", str(inc)] if inc else []
    # Per-run extra compile flags (e.g. a raised -fconstexpr-steps for heavy
    # reflection like nlohmann/json's basic_json). Recorded in meta.toml for
    # reproducibility; appended to the binding (Gate 4) compile only.
    extra_cflags = meta.get("extra_cflags", [])
    mod = meta["module_name"]
    strategy = meta.get("strategy", "single_stage")

    r = {
        "schema_version": 1,
        "slug": meta["slug"],
        "url": meta.get("url", ""),
        "pinned_commit": meta.get("pin", ""),
        "tier": meta.get("tier"),
        "header_only": meta.get("header_only", True),
        "toolchain_commit": sh(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD:llvm-project"]).stdout.strip() or "unknown",
        "binder_commit": sh(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD:nanobind"]).stdout.strip() or "unknown",
        "outcome": None,
        "furthest_gate": 0,
        "gate_results": {"3_strategy": strategy},
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

    build_dir = run / "binding" / "build"

    # ---- Gate 1: probe ----
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

    # ---- Gate 4: compile the binding ----
    t0 = time.time()
    if strategy == "two_stage":
        b = sh(["bash", str(LIB / "build_module_codegen.sh"),
                str(run / "binding" / "gen.cpp"), str(run / "binding" / "binding.cpp"),
                mod, str(build_dir), *incflags, *extra_cflags])
    else:
        b = sh(["bash", str(LIB / "build_module.sh"),
                str(run / "binding" / "binding.cpp"), mod, str(build_dir), *incflags, *extra_cflags])
    r["metrics"]["binding_compile_seconds"] = round(time.time() - t0, 2)
    if b.returncode != 0:
        r["gate_results"]["4_compile"] = "fail"
        r["outcome"] = "B"
        stage = next((l.split("=", 1)[1] for l in b.stderr.splitlines()
                      if l.startswith("BUILD_FAIL_STAGE=")), "module")
        r["failure"]["code"] = f"B.{stage}"
        r["failure"]["first_diagnostics"] = first_lines(b.stderr + b.stdout)
        r["toolchain_bug"] = detect_toolchain_bug(b.stderr + b.stdout)
        return finish(run, r)
    r["gate_results"]["4_compile"] = "pass"
    r["furthest_gate"] = 4

    # ---- Gate 5: import smoke ----
    imp = sh([str(VENV_PY), "-c", f"import {mod}"],
             env={**run_env(), "PYTHONPATH": str(build_dir)})
    if imp.returncode != 0:
        r["gate_results"]["5_import"] = "fail"
        r["outcome"] = "C"
        sig = "SIGSEGV" if imp.returncode in (-11, 139) else ("Import" if "Error" in imp.stderr else str(imp.returncode))
        r["failure"]["code"] = f"C.{sig.lower()}"
        r["failure"]["signal"] = sig
        r["failure"]["first_diagnostics"] = first_lines(imp.stderr)
        return finish(run, r)
    r["gate_results"]["5_import"] = "pass"
    r["furthest_gate"] = 5

    # ---- Gate 6: correctness ----
    test_py = run / "tests" / "test_bindings.py"
    oracle = run / "tests" / "oracle_native.cpp"
    has_diff = oracle.exists()
    if has_diff:
        # build native oracle (-O2, per TC-0001) and emit ground-truth expected.json
        ob = sh(["bash", str(LIB / "build_native.sh"), str(oracle),
                 str(run / "tests" / "build" / "oracle"), *incflags])
        if ob.returncode == 0:
            res = sh([str(run / "tests" / "build" / "oracle")], env=run_env())
            if res.returncode == 0 and res.stdout.strip():
                (run / "tests" / "expected.json").write_text(res.stdout)
                r["oracle"]["layers_used"].append("L1_differential")
                r["oracle"]["ground_truth_source"] = "native C++ oracle (oracle_native.cpp)"
            else:
                r["notes"] += "oracle ran but produced no output; "
        else:
            r["notes"] += "native oracle failed to build; "

    if not test_py.exists():
        r["outcome"] = "E-weak"
        r["gate_results"]["6_correct"] = "skip"
        r["notes"] += "no test_bindings.py (import-only). "
        r["furthest_gate"] = 6
        return finish(run, r)

    t = sh([str(VENV_PY), "-m", "pytest", str(test_py), "-q",
            "-o", "cache_dir=/tmp/.pytest_corpus"],
           env={**run_env(), "PYTHONPATH": str(build_dir)})
    r["gate_results"]["6_correct"] = "pass" if t.returncode == 0 else "fail"
    # parse "N passed" / "N failed"
    tail = t.stdout.strip().splitlines()[-1] if t.stdout.strip() else ""
    r["oracle"]["pytest_summary"] = tail
    if t.returncode != 0:
        r["outcome"] = "D"
        r["failure"]["code"] = "D.value"
        r["failure"]["first_diagnostics"] = first_lines(t.stdout, 8)
        r["furthest_gate"] = 6
        return finish(run, r)
    r["furthest_gate"] = 6
    # E if a differential (L1/L2) layer was used; else E-weak (invariants/import only)
    r["outcome"] = "E" if (has_diff and "L1_differential" in r["oracle"]["layers_used"]) else "E-weak"
    return finish(run, r)


def finish(run, r):
    (run / "result.json").write_text(json.dumps(r, indent=2) + "\n")
    print(f"[{r['slug']}] outcome={r['outcome']} gate={r['furthest_gate']} "
          f"{r['failure']['code'] or ''} {r['oracle'].get('pytest_summary','')}".strip())
    if r["toolchain_bug"].get("status", "none") != "none":
        print(f"  toolchain_bug: {r['toolchain_bug']['status']} "
              f"({r['toolchain_bug'].get('kind')}) {r['toolchain_bug'].get('finding','')}")
    return 0 if r["outcome"] in ("E", "E-weak") else 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: run_gates.py <run_dir>", file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
