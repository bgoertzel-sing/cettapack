#!/usr/bin/env python3
"""cetta-internals P: the calibration prototype (0012) per timed function: timers build vs timers build + 0012.

  proto_sites.py EV_DIR EVPROTO_DIR [--md OUT.md] [--json OUT.json] [--top N]

Per workload and run: each timed function's corrected self seconds (calibration as evprof_report.py) and calls; the
table gives the median over runs of the seconds with and without the prototype, the change, and whether the call
counts are identical (they must be: the prototype changes no decision).  Also the change of the corrected total.
"""
import argparse
import glob
import json
import os
import re
import statistics

WORKLOADS = ["w1-examples", "w2-wmtm", "w3-benchgen"]
RX = re.compile(r"^(?P<name>.+)\.(?P<tag>[^.]+)\.r(?P<r>\d+)\.evprof\.json$")


def load(d):
    runs = {}
    for w in WORKLOADS:
        for p in glob.glob(os.path.join(d, w, "*.evprof.json")):
            m = RX.match(os.path.basename(p))
            j = json.load(open(p))
            c = j["calibration"]
            cin, cout, hz = c["c_in_median"], c["c_out_median"], j["tsc_hz"]
            agg = runs.setdefault((w, m.group("r")), {"total": 0.0, "sites": {}})
            agg["total"] += (j["total_ticks"] - sum(s["calls"] for s in j["sites"]) * (cin + cout)) / hz
            for s in j["sites"]:
                x = agg["sites"].setdefault(s["name"], [0.0, 0])
                x[0] += (s["self"] - s["calls"] * cin - s["nchild"] * cout) / hz
                x[1] += s["calls"]
    return runs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ev")
    ap.add_argument("evproto")
    ap.add_argument("--md")
    ap.add_argument("--json")
    ap.add_argument("--top", type=int, default=8)
    a = ap.parse_args()
    A, B = load(a.ev), load(a.evproto)
    out = {}
    lines = []
    for w in WORKLOADS:
        keys = sorted(r for (ww, r) in A if ww == w and (ww, r) in B)
        if not keys:
            continue
        names = A[(w, keys[0])]["sites"].keys()
        same_calls = all(A[(w, r)]["sites"][n][1] == B[(w, r)]["sites"][n][1] for r in keys for n in names
                         if n not in ("malloc", "calloc", "realloc", "free", "arena_grow"))
        rows = []
        for n in names:
            x = statistics.median(A[(w, r)]["sites"][n][0] for r in keys)
            y = statistics.median(B[(w, r)]["sites"][n][0] for r in keys)
            rows.append((y - x, n, x, y))
        rows.sort()
        tot_a = statistics.median(A[(w, r)]["total"] for r in keys)
        tot_b = statistics.median(B[(w, r)]["total"] for r in keys)
        out[w] = {"runs": keys, "same_calls": same_calls, "total": [tot_a, tot_b],
                  "sites": {n: [x, y] for d, n, x, y in rows}}
        lines.append("**%s** (runs %s; call counts identical outside the allocator: %s): corrected total %.2f s -> "
                     "%.2f s (%+.1f%%)" % (w, ",".join(keys), "yes" if same_calls else "NO", tot_a, tot_b,
                                         100 * (tot_b / tot_a - 1)))
        lines.append("")
        lines.append("| function | without 0012 (s) | with 0012 (s) | change |")
        lines.append("|---|---|---|---|")
        for d, n, x, y in rows[:a.top]:
            lines.append("| %s | %.3f | %.3f | %+.3f (%+.0f%%) |" % (n, x, y, d, 100 * d / x if x else 0))
        lines.append("")
    text = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(text)
    else:
        print(text)
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
