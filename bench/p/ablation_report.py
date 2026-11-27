#!/usr/bin/env python3
"""cetta-internals P: end-to-end time of the calibration runs against their base, paired by run.

  ablation_report.py DIR [--base base] [--json OUT.json] [--md OUT.md]

DIR holds CONFIG/WORKLOAD/NAME.TAG.rN.{time,check} (bench/p/run_ablations.sh or run_all.sh); TAG is anything.
For each configuration and workload: user CPU seconds summed over the workload's files per run, the ratio
config/base paired by run number (median, min-max), and the answer check (file-runs whose stdout equals S0's).
"""
import argparse
import collections
import glob
import json
import os
import re
import statistics

WORKLOADS = ["w1-examples", "w2-wmtm", "w3-benchgen"]
RX = re.compile(r"^(?P<name>.+)\.(?P<tag>[^.]+)\.r(?P<r>\d+)\.time$")


def load(d):
    t = collections.defaultdict(lambda: collections.defaultdict(float))  # w -> r -> user
    files = collections.defaultdict(lambda: collections.defaultdict(int))
    checks = collections.Counter()
    for w in WORKLOADS:
        for p in glob.glob(os.path.join(d, w, "*.time")):
            m = RX.match(os.path.basename(p))
            if not m:
                continue
            fields = open(p).read().split()
            if len(fields) < 4 or not os.path.exists(p[:-5] + ".check"):
                continue  # a run still in progress
            e, u, s, mem = fields[-4:]
            t[w][m.group("r")] += float(u)
            files[w][m.group("r")] += 1
            c = open(p[:-5] + ".check").read()
            checks["same" if "answers_vs_s0_base=same" in c and "failed_tests=0" in c and "exit=0" in c
                   else "DIFFERENT"] += 1
    return t, files, checks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--base", default="base")
    ap.add_argument("--json")
    ap.add_argument("--md")
    a = ap.parse_args()
    configs = sorted(x for x in os.listdir(a.dir) if os.path.isdir(os.path.join(a.dir, x)))
    data = {c: load(os.path.join(a.dir, c)) for c in configs}
    bt, bf, _ = data[a.base]
    res = {}
    lines = ["| configuration | answers (file-runs same as S0) | " + " | ".join(
        "%s user s (median) / ratio to %s, median (min-max)" % (w, a.base) for w in WORKLOADS) + " |",
             "|---|---|" + "---|" * len(WORKLOADS)]
    for c in configs:
        t, f, checks = data[c]
        cells = []
        res[c] = {"checks": dict(checks), "workloads": {}}
        for w in WORKLOADS:
            runs = sorted(set(t[w]) & set(bt[w]))
            ratios = [t[w][r] / bt[w][r] for r in runs if f[w][r] == bf[w][r] and bt[w][r] > 0]
            med = statistics.median(ratios) if ratios else float("nan")
            secs = statistics.median([t[w][r] for r in runs]) if runs else float("nan")
            res[c]["workloads"][w] = {"user_s": {r: t[w][r] for r in runs}, "ratios": ratios, "median": med}
            cells.append("%.1f / %.3f (%.3f-%.3f)" % (secs, med, min(ratios), max(ratios)) if ratios else "n/a")
        lines.append("| %s | %d/%d | %s |" % (c, checks["same"], sum(checks.values()), " | ".join(cells)))
    text = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(text)
    else:
        print(text)
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
