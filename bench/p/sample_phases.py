#!/usr/bin/env python3
"""cetta-internals P: the evaluator phases measured a second way, from S1's C-level sampler (cross-check of the timers).

  sample_phases.py SITES_TXT SYM.json [SYM.json ...] --label W [--json OUT] [--top N]

SYM.json: one symbolised sample file per file-run (NAME.TAG.rN.sym.json; a workload's files are pooled per run N) (bench/s1/analyze_samples.py symbolise; stacks with inlined frames).
Each sample gets the phase of the innermost function on its stack that the timers time (SITES_TXT), the way the
timers charge time; a sample inside libc's allocator (malloc, free, calloc, realloc, and the two internal entry
points the stripped libc resolves to the nearest exported symbol, __default_morecore and timer_settime, which the
caller lists show are allocator paths) is "alloc"; other libc leaves (string functions) go to the enclosing
function, as with the timers.  Samples in the profiler's own code are dropped.  Also reports S1's native categories
(bench/s1/sample_categories.py) and the top leaf functions per phase, so the self time of large timed functions can
be split into what they inline.
"""
import argparse
import collections
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "s1"))
import sample_categories as sc  # noqa: E402

ALLOC_LIBC = {"malloc", "free", "calloc", "realloc", "__default_morecore", "timer_settime", "brk", "sbrk",
              "__libc_malloc", "__libc_free", "cfree", "_int_malloc", "_int_free", "malloc_consolidate"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sites")
    ap.add_argument("sym", nargs="+")
    ap.add_argument("--label", default="")
    ap.add_argument("--json")
    ap.add_argument("--top", type=int, default=12)
    a = ap.parse_args()
    phase_of = {}
    for ln in open(a.sites):
        ln = ln.split("#", 1)[0].strip()
        if ln:
            ph, fn = ln.split()
            phase_of[fn] = ph
    per_run = collections.OrderedDict()  # run number -> phase counts (a workload's files pooled per run)
    leaf_by_phase = collections.defaultdict(collections.Counter)
    site_self = collections.Counter()
    cats = collections.Counter()
    callers = collections.defaultdict(collections.Counter)  # innermost site -> next timed function outwards
    total = 0
    for path in a.sym:
        d = json.load(open(path))
        m = re.search(r"\.r(\d+)\.sym\.json$", path)
        ph = per_run.setdefault(m.group(1) if m else path, collections.Counter())
        n = 0
        for s in d["samples"]:
            st = sc.trim(s["stack"])
            if not st or not st[0]:
                continue
            fns = [sc.norm(fn) for fr in st for fn in fr]
            if any(x.startswith(("fnprof_", "cetta_fnprof")) for x in fns[:3]):
                continue
            n += 1
            leaf = fns[0]
            phase, site, at = None, None, -1
            if "libc.so" in leaf and leaf.split("@")[0] in ALLOC_LIBC:
                phase, site = "alloc", "(libc allocator)"
            else:
                for i, fn in enumerate(fns):
                    base = fn.split("@")[0]
                    if base in phase_of:
                        phase, site, at = phase_of[base], base, i
                        break
            outer = next((fn.split("@")[0] for fn in fns[at + 1:] if fn.split("@")[0] in phase_of
                          and fn.split("@")[0] != site), "(none)")
            callers[site][outer] += 1
            if phase is None:
                phase, site = "outside", "(outside)"
            ph[phase] += 1
            site_self[site] += 1
            leaf_by_phase[phase][leaf] += 1
            cats[sc.categorise(s["stack"])] += 1
        total += n
    per_run = [{k: v / sum(c.values()) for k, v in c.items()} for c in per_run.values()]
    phases = sorted({k for r in per_run for k in r})
    out = {"label": a.label, "samples": total, "runs": len(per_run),
           "phase_share_median": {}, "phase_share_range": {}, "site_self": {}, "categories": {}, "leaf_by_phase": {}}
    print("## %s: sampler phases (%d samples, %d runs)" % (a.label, total, len(per_run)))
    for k in phases:
        xs = sorted(r.get(k, 0.0) for r in per_run)
        m = xs[len(xs) // 2]
        out["phase_share_median"][k] = m
        out["phase_share_range"][k] = [xs[0], xs[-1]]
        print("  %-12s %5.1f%% (%4.1f-%4.1f)" % (k, 100 * m, 100 * xs[0], 100 * xs[-1]))
    for k, v in site_self.most_common():
        out["site_self"][k] = v / total
    for k, v in cats.most_common():
        out["categories"][k] = v / total
    print("  top functions (innermost timed function), share of samples:")
    for k, v in site_self.most_common(25):
        print("    %5.2f%% %s" % (100 * v / total, k))
    for k in phases:
        c = leaf_by_phase[k]
        tot = sum(c.values())
        out["leaf_by_phase"][k] = [[f, v / total] for f, v in c.most_common(a.top)]
    out["callers"] = {k: [[f, v / total] for f, v in c.most_common(8)] for k, c in callers.items()}
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
