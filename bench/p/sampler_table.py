#!/usr/bin/env python3
"""cetta-internals P: the sampler's phase shares as a table next to the timers' (cross-check).

  sampler_table.py SAMPLER_DIR EVPROF.json [--md OUT.md]

SAMPLER_DIR: sample_phases.WORKLOAD.json (bench/p/sample_phases.py); EVPROF.json: bench/p/evprof_report.py --json.
"""
import argparse
import json
import os

WORKLOADS = ["w1-examples", "w2-wmtm", "w3-benchgen"]
PHASES = ["dispatch", "choice", "eqsel", "prepcompile", "prepexec", "match", "copy", "alloc", "space", "prolog",
          "outside"]
CATS = None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sdir")
    ap.add_argument("evprof")
    ap.add_argument("--md")
    a = ap.parse_args()
    ev = json.load(open(a.evprof))["workloads"]
    sp = {w: json.load(open(os.path.join(a.sdir, "sample_phases.%s.json" % w))) for w in WORKLOADS}
    lines = ["| phase | " + " | ".join("%s timers / sampler" % w for w in WORKLOADS) + " |",
             "|---|" + "---|" * len(WORKLOADS)]
    for p in PHASES:
        cells = []
        for w in WORKLOADS:
            xs = sorted(ev[w]["phase_shares"].get(p, [0.0]))
            t = xs[len(xs) // 2]
            s = sp[w]["phase_share_median"].get(p, 0.0)
            cells.append("%.1f%% / %.1f%%" % (100 * t, 100 * s))
        lines.append("| %s | %s |" % (p, " | ".join(cells)))
    lines.append("| samples (3 runs) | %s |" % " | ".join("%d" % sp[w]["samples"] for w in WORKLOADS))
    lines += ["", "S1's native categories (innermost named function, bench/s1/sample_categories.py), same samples:", "",
              "| category | " + " | ".join(WORKLOADS) + " |", "|---|" + "---|" * len(WORKLOADS)]
    cats = sorted({c for w in WORKLOADS for c in sp[w]["categories"]},
                  key=lambda c: -sum(sp[w]["categories"].get(c, 0) for w in WORKLOADS))
    for c in cats:
        lines.append("| %s | %s |" % (c, " | ".join("%.1f%%" % (100 * sp[w]["categories"].get(c, 0)) for w in WORKLOADS)))
    text = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(text)
    else:
        print(text)


if __name__ == "__main__":
    main()
