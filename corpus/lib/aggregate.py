#!/usr/bin/env python3
"""Roll up every corpus/runs/*/result.json into the campaign-level deliverables:

  aggregate/results.jsonl     one result per line (the machine-readable record)
  aggregate/report.md         outcome histogram per tier, E/E-weak lists, failure clusters
  aggregate/toolchain_bugs.md toolchain_bug blocks grouped by signature/finding (upstream queue)

Usage:  python aggregate.py
"""
import collections
import json
import pathlib

CORPUS = pathlib.Path(__file__).resolve().parent.parent
RUNS = CORPUS / "runs"
AGG = CORPUS / "aggregate"

OUTCOME_LABEL = {
    "A": "won't compile under toolchain",
    "B": "binding failed to compile",
    "C": "import crashed",
    "D": "behavior wrong",
    "E": "clean success (behavioral)",
    "E-weak": "success (import/invariant only)",
}


def main():
    AGG.mkdir(exist_ok=True)
    results = []
    for rj in sorted(RUNS.glob("*/result.json")):
        results.append(json.loads(rj.read_text()))

    (AGG / "results.jsonl").write_text("".join(json.dumps(r) + "\n" for r in results))

    # ---- report.md ----
    by_tier = collections.defaultdict(lambda: collections.Counter())
    overall = collections.Counter()
    for r in results:
        by_tier[r.get("tier")][r["outcome"]] += 1
        overall[r["outcome"]] += 1

    lines = ["# Corpus report", "",
             f"Total runs: **{len(results)}**", ""]
    lines.append("## Outcomes (overall)")
    lines.append("")
    lines.append("| outcome | count | meaning |")
    lines.append("|---|---|---|")
    for o in ["E", "E-weak", "D", "C", "B", "A"]:
        if overall[o]:
            lines.append(f"| {o} | {overall[o]} | {OUTCOME_LABEL[o]} |")
    lines.append("")
    lines.append("## Outcomes by tier")
    lines.append("")
    lines.append("| tier | " + " | ".join(["E", "E-weak", "D", "C", "B", "A"]) + " |")
    lines.append("|---|" + "---|" * 6)
    for tier in sorted(by_tier, key=lambda t: (t is None, t)):
        c = by_tier[tier]
        lines.append(f"| {tier} | " + " | ".join(str(c[o]) for o in
                     ["E", "E-weak", "D", "C", "B", "A"]) + " |")
    lines.append("")
    lines.append("## Per-run")
    lines.append("")
    lines.append("| slug | tier | outcome | constexpr | emit | surface | gate | strategy | pin | notes |")
    lines.append("|---|---|---|---|---|---|---|---|---|---|")
    for r in sorted(results, key=lambda r: (r.get("tier") or 0, r["slug"])):
        fc = (r.get("failure") or {}).get("code") or ""
        lanes = r.get("lanes") or {}
        cx = (lanes.get("constexpr") or {}).get("outcome", r["outcome"])
        em = (lanes.get("emit") or {}).get("outcome", "-")
        sd = (r.get("surface_diff") or {}).get("status", "-")
        lines.append(f"| {r['slug']} | {r.get('tier')} | {r['outcome']} | "
                     f"{cx} | {em} | {sd} | "
                     f"{r['furthest_gate']} | {r['gate_results'].get('3_strategy','')} | "
                     f"{r.get('pinned_commit','')} | {fc} {r.get('notes','').strip()[:90]} |")
    lines.append("")

    # ---- emit-lane section (schema v2): rollout status + failure clusters ----
    em_runs = [r for r in results if (r.get("lanes") or {}).get("emit")]
    lines.append("## Emit lane (production-toolchain source codegen)")
    lines.append("")
    if not em_runs:
        lines.append("_No run has an emit lane yet (meta.toml [emit])._")
    else:
        em_counts = collections.Counter(
            r["lanes"]["emit"]["outcome"] for r in em_runs)
        sd_fail = [r["slug"] for r in em_runs
                   if (r.get("surface_diff") or {}).get("status") == "fail"]
        lines.append(f"Runs with an emit lane: **{len(em_runs)}** / {len(results)}; "
                     f"outcomes: " + ", ".join(f"{k}={v}" for k, v in sorted(em_counts.items())))
        if sd_fail:
            lines.append(f"Surface-diff failures: {', '.join(sd_fail)}")
        clusters = collections.defaultdict(list)
        for r in em_runs:
            code = (r["lanes"]["emit"].get("failure") or {}).get("code")
            if code:
                clusters[code].append(r["slug"])
        for code, slugs in sorted(clusters.items(), key=lambda kv: -len(kv[1])):
            lines.append(f"- `{code}`: {len(slugs)} run(s) -- {', '.join(slugs)}")
    lines.append("")
    (AGG / "report.md").write_text("\n".join(lines) + "\n")

    # ---- toolchain_bugs.md (grouped, impact-ranked) ----
    groups = collections.defaultdict(list)
    for r in results:
        tb = r.get("toolchain_bug") or {}
        if tb.get("status", "none") != "none":
            key = tb.get("finding") or tb.get("signature") or tb.get("kind") or "unknown"
            groups[key].append((r["slug"], tb))
    tlines = ["# Toolchain bugs (upstream queue)", "",
              "Grouped by signature/finding, ranked by repo-hit-count. "
              "Each distinct group is one upstream item.", ""]
    if not groups:
        tlines.append("_No toolchain bugs recorded in result.json yet. "
                      "(Standalone findings live in corpus/findings/.)_")
    for key, hits in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        status = hits[0][1].get("status")
        kind = hits[0][1].get("kind")
        tlines.append(f"## {key}  —  {len(hits)} repo(s), {status} ({kind})")
        tlines.append("- hit by: " + ", ".join(s for s, _ in hits))
        tlines.append("")
    (AGG / "toolchain_bugs.md").write_text("\n".join(tlines) + "\n")

    print(f"aggregated {len(results)} runs -> {AGG}/report.md, results.jsonl, toolchain_bugs.md")
    print("outcomes:", dict(overall))


if __name__ == "__main__":
    main()
