#!/usr/bin/env python3
"""S1 step 4: where the time goes inside PeTTaChainer's search (frontier/agenda) and proof-store code, from the
C-level samples of the profiled runs (upstream/cetta-s1/0006; symbolised by analyze_samples.py symbolise).

Rows (the profiler's MeTTa rows) are assigned to the PeTTaChainer module that defines them (the file holding the
row's equations in the image; a match@F row goes with F).  The search episode of a query is the backward chainer:
rows of backward_chainer.metta and backward_proof_store.metta ("search").  For each workload:

  - the share of all samples by module of the sample's own row (self), and the inclusive share of the search
    episode (samples with a search row anywhere on the row chain: the search code and everything it calls);
  - inside the search rows themselves (self) and inside the episode (inclusive): the native categories
    (bench/s1/sample_categories.py) and the top native functions;
  - the profiler's own samples (fnprof_*) are counted and reported, and all shares are given with them removed
    (they do not exist in an uninstrumented run).

  step4_search.py --image IMAGE_DIR --sym W=FILE.sym.json [--sym W=...] --out-md OUT.md --out-json OUT.json
"""
import argparse
import collections
import glob
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sample_categories as sc  # noqa: E402

SEARCH_MODULES = ("backward_chainer.metta", "backward_proof_store.metta")
HEAD = re.compile(r"^\(=\s+\(([^\s()]+)")


def module_map(image):
    m = {}
    files = sorted(glob.glob(os.path.join(image, "*.metta"))) + sorted(glob.glob(os.path.join(image, "lib", "*.metta")))
    for f in files:
        name = os.path.relpath(f, image)
        for ln in open(f, errors="replace"):
            h = HEAD.match(ln)
            if h and h.group(1) not in m:
                m[h.group(1)] = name
    return m


def row_module(row, mm):
    if row == "<outside-equations>":
        return "<outside any equation>"
    if row.startswith("match@"):
        f = row[len("match@"):].split(" ", 1)[0]
        return mm.get(f, "<other>")
    return mm.get(row, "<other>")


def pct(n, d):
    return 100.0 * n / d if d else 0.0


def analyse(paths, mm):
    total = prof = 0
    by_module = collections.Counter()
    search_self_cat = collections.Counter()
    search_incl_cat = collections.Counter()
    search_self_fn = collections.Counter()
    search_incl = 0
    all_cat = collections.Counter()
    rows_self = collections.Counter()
    for p in paths:
        for s in json.load(open(p))["samples"]:
            total += 1
            cat = sc.categorise(s["stack"])
            if cat == "profiler":
                prof += 1
                continue
            all_cat[cat] += 1
            mods = [row_module(r, mm) for r in s["rows"]]
            by_module[mods[0]] += 1
            rows_self[s["rows"][0]] += 1
            if mods[0] in SEARCH_MODULES:
                search_self_cat[cat] += 1
                search_self_fn[sc.leaf_function(s["stack"])] += 1
            if any(m in SEARCH_MODULES for m in mods):
                search_incl += 1
                search_incl_cat[cat] += 1
    n = total - prof
    return {
        "samples": total, "profiler_samples": prof, "samples_without_profiler": n,
        "by_module_self": dict(by_module), "categories_all": dict(all_cat),
        "search_self": sum(search_self_cat.values()), "search_self_categories": dict(search_self_cat),
        "search_self_top_functions": search_self_fn.most_common(25),
        "search_incl": search_incl, "search_incl_categories": dict(search_incl_cat),
        "top_rows_self": rows_self.most_common(30),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True)
    ap.add_argument("--sym", action="append", required=True, help="WORKLOAD=glob of sym.json files")
    ap.add_argument("--fnprof", action="append", default=[], help="WORKLOAD=glob of fnprof.json files")
    ap.add_argument("--out-md", required=True)
    ap.add_argument("--out-json", required=True)
    a = ap.parse_args()
    mm = module_map(a.image)
    res = {}
    for spec in a.sym:
        w, _, g = spec.partition("=")
        paths = sorted(glob.glob(g))
        res[w] = analyse(paths, mm)
        res[w]["files"] = [os.path.basename(p) for p in paths]
    for spec in a.fnprof:
        w, _, g = spec.partition("=")
        tot = collections.Counter()
        for p in sorted(glob.glob(g)):
            for fn in json.load(open(p))["functions"]:
                head = fn["head"]
                mod = row_module(head, mm)
                if mod not in SEARCH_MODULES:
                    continue
                kind = "match sites" if head.startswith("match@") else "functions"
                tot[kind + ".calls"] += fn["calls"] + fn.get("prepared_calls", 0) * 0
                tot[kind + ".candidates"] += fn["candidates"]
                tot[kind + ".results"] += fn["results"]
                tot[kind + ".frames"] += fn["frames"]
                if kind == "functions" and fn["candidates"] > fn["calls"]:
                    tot["functions.with_more_candidates_than_calls"] += 1
                    tot["functions.extra_candidates"] += fn["candidates"] - fn["calls"]
                if kind == "functions":
                    tot["functions.rows"] += 1
        res.setdefault(w, {})["search_counts"] = dict(tot)
    json.dump(res, open(a.out_json, "w"), indent=1, sort_keys=True)
    cats = [c for c, _ in sc.RULES if c != "profiler"] + ["other machine"]
    L = []
    L.append("# S1 step 4: search episode sizing from C-level samples (generated by bench/s1/step4_search.py)\n")
    L.append("Samples: SIGPROF every 1 ms of process CPU time, native backtrace + profiler row "
             "(upstream/cetta-s1/0006). Shares exclude the profiler's own samples (its bookkeeping does not exist "
             "in an uninstrumented run). Search = rows defined in %s.\n" % " and ".join(SEARCH_MODULES))
    L.append("## Where the samples are, by PeTTaChainer module (self)\n")
    mods = sorted({m for r in res.values() for m in r["by_module_self"]},
                  key=lambda m: -sum(r["by_module_self"].get(m, 0) for r in res.values()))
    L.append("| module | " + " | ".join(res) + " |")
    L.append("|---|" + "---|" * len(res))
    for m in mods:
        L.append("| %s | " % m + " | ".join("%.1f%%" % pct(r["by_module_self"].get(m, 0), r["samples_without_profiler"])
                                          for r in res.values()) + " |")
    L.append("| (profiler's own samples, removed) | " + " | ".join(
        "%.1f%% of all" % pct(r["profiler_samples"], r["samples"]) for r in res.values()) + " |")
    L.append("| samples (without profiler) | " + " | ".join(str(r["samples_without_profiler"]) for r in res.values()) + " |")
    L.append("\n## The search episode: share of run time, and what it is made of\n")
    L.append("| | " + " | ".join(res) + " |")
    L.append("|---|" + "---|" * len(res))
    L.append("| search rows, self | " + " | ".join("%.1f%%" % pct(r["search_self"], r["samples_without_profiler"])
                                                  for r in res.values()) + " |")
    L.append("| search episode, inclusive (search rows and all they call) | " + " | ".join(
        "%.1f%%" % pct(r["search_incl"], r["samples_without_profiler"]) for r in res.values()) + " |")
    for label, key, base in (("inside the search rows (self), by native category", "search_self_categories", "search_self"),
                             ("inside the search episode (inclusive), by native category", "search_incl_categories", "search_incl"),
                             ("whole run, by native category", "categories_all", "samples_without_profiler")):
        L.append("\n### %s (share of that part)\n" % label)
        L.append("| category | " + " | ".join(res) + " |")
        L.append("|---|" + "---|" * len(res))
        for c in cats:
            L.append("| %s | " % c + " | ".join("%.1f%%" % pct(r[key].get(c, 0), r[base]) for r in res.values()) + " |")
    L.append("\n### Top native functions (leaf) inside the search rows\n")
    for w, r in res.items():
        L.append("- **%s**: " % w + ", ".join("%s %.1f%%" % (f, pct(n, r["search_self"])) for f, n in r["search_self_top_functions"][:15]))
    if any("search_counts" in r for r in res.values()):
        L.append("\n### Calls and equation candidates in the search rows (profiler counts)\n")
        L.append("| | " + " | ".join(res) + " |")
        L.append("|---|" + "---|" * len(res))
        for k in ("functions.rows", "functions.calls", "functions.candidates", "functions.extra_candidates",
                  "functions.with_more_candidates_than_calls", "functions.frames", "functions.results",
                  "match sites.calls", "match sites.candidates", "match sites.results"):
            L.append("| %s | " % k + " | ".join("{:,}".format(r.get("search_counts", {}).get(k, 0)) for r in res.values()) + " |")
    L.append("\nFiles: " + "; ".join("%s: %s" % (w, ", ".join(r["files"])) for w, r in res.items()))
    open(a.out_md, "w").write("\n".join(L) + "\n")
    print("\n".join(L))


if __name__ == "__main__":
    main()
