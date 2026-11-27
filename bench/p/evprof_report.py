#!/usr/bin/env python3
"""cetta-internals P: phase shares from the evaluator timers (upstream/cetta-p/0011, CETTA_EVPROF=1).

  evprof_report.py RUNDIR [--off OFFDIR] [--cal min|median] [--json OUT.json] [--md OUT.md]

RUNDIR holds WORKLOAD/NAME.ev.rN.evprof.json (bench/p/run_workloads.sh, BUILD=ev); OFFDIR the same files' runs of
the timers-off build (NAME.off.rN.time: elapsed user sys maxrss).

Overhead subtraction (cetta_evprof.c): every timed frame costs c_in ticks inside itself and c_out ticks in its
parent, both measured in-process at exit on empty frames (minimum, or median, of 21 rounds).  Corrected self of a
site = self - calls*c_in - nchild*c_out; corrected "outside" (time in no timed frame) = total - top_incl -
top_frames*c_out, top_frames = calls - sum(nchild).  Corrected total = total - calls*(c_in + c_out).  The
residual overhead = corrected total / wall time of the timers-off run of the same file and run number - 1: the
part of the instrumentation cost that calibration does not explain (cache effects, the larger code), reported,
not hidden.  Shares are of the corrected total, summed over a workload's files, per run; the table gives the
median over runs and the min-max.
"""
import argparse
import collections
import glob
import json
import os
import re
import statistics

PHASE_ORDER = ["dispatch", "choice", "eqsel", "prepcompile", "prepexec", "match", "copy", "alloc", "space",
               "prolog", "outside"]
PHASE_LABEL = {
    "dispatch": "goal dispatch and evaluation loop (incl. open-equation machine)",
    "choice": "choice points, backtracking, answer collection",
    "eqsel": "equation lookup and selection, call preparation, admission checks",
    "prepcompile": "prepared-program compilation (call preparation that builds code)",
    "prepexec": "running prepared programs and prepared queries",
    "match": "unification and matching, bindings",
    "copy": "term copying and instantiation",
    "alloc": "allocation and freeing (malloc/calloc/realloc/free, arena growth)",
    "space": "space access (space queries, equation cursors, add/remove)",
    "prolog": "SWI-Prolog calls and term conversion",
    "outside": "outside every timed function (loading, parsing, top level)",
}
# the order's five phases (plus the rest), for the headline table
GROUP = {"eqsel": "equation lookup and selection", "prepcompile": "equation lookup and selection",
         "match": "unification and matching", "dispatch": "dispatch", "choice": "dispatch",
         "prepexec": "dispatch", "alloc": "term allocation and copying", "copy": "term allocation and copying",
         "space": "space access", "prolog": "Prolog (foreign calls)", "outside": "outside"}
GROUP_ORDER = ["equation lookup and selection", "unification and matching", "dispatch",
               "term allocation and copying", "space access", "Prolog (foreign calls)", "outside"]
WORKLOADS = ["w1-examples", "w2-wmtm", "w3-benchgen"]
RUN = re.compile(r"^(?P<name>.+)\.(?P<tag>[a-z0-9-]+)\.r(?P<r>\d+)\.evprof\.json$")


def load_run(path, cal):
    d = json.load(open(path))
    c = d["calibration"]
    c_in = c["c_in_min"] if cal == "min" else c["c_in_median"]
    c_out = c["c_out_min"] if cal == "min" else c["c_out_median"]
    sites = d["sites"]
    calls = sum(s["calls"] for s in sites)
    nchild = sum(s["nchild"] for s in sites)
    top_frames = calls - nchild
    hz = d["tsc_hz"]
    out = {"sites": {}, "phases": collections.Counter()}
    for s in sites:
        corr = s["self"] - s["calls"] * c_in - s["nchild"] * c_out
        out["sites"][s["name"]] = {"phase": s["phase"], "calls": s["calls"], "self_raw": s["self"] / hz,
                                   "self": corr / hz, "incl_raw": s["incl"] / hz}
        out["phases"][s["phase"]] += corr / hz
    # Every tick inside a timed frame is the self time of exactly one site, so the time outside all frames is the
    # total minus the sites' raw self time.  (The report's top_incl_ticks also holds the calibration frames, which run
    # after the run's last stamp, so it is not used.)
    inside = sum(s["self"] for s in sites)
    outside = d["total_ticks"] - inside - top_frames * c_out
    out["phases"]["outside"] += outside / hz
    out["covered"] = inside / d["total_ticks"]
    out["total_raw"] = d["total_ticks"] / hz
    out["total"] = (d["total_ticks"] - calls * (c_in + c_out)) / hz
    out["frames"] = calls
    out["user"] = d["utime_us"] / 1e6
    out["cal"] = (c_in, c_out)
    out["unwound"] = d["unwound"]
    out["alloc_bytes"] = d["alloc_bytes_requested"]
    return out


def off_time(offdir, w, name, r):
    p = os.path.join(offdir, w, "%s.off.r%s.time" % (name, r))
    if not os.path.exists(p):
        return None
    e, u, s, m = open(p).read().split()[-4:]
    return float(e), float(u)


def summarize(rundir, offdir, cal):
    per = {}  # w -> r -> aggregate
    for w in WORKLOADS:
        for path in sorted(glob.glob(os.path.join(rundir, w, "*.evprof.json"))):
            m = RUN.match(os.path.basename(path))
            if not m:
                continue
            r = m.group("r")
            run = load_run(path, cal)
            agg = per.setdefault(w, {}).setdefault(r, {"phases": collections.Counter(), "sites": {},
                                                         "total": 0.0, "total_raw": 0.0, "frames": 0,
                                                         "off_wall": 0.0, "off_user": 0.0, "ev_user": 0.0,
                                                         "files": 0, "off_missing": 0, "cal": [],
                                                         "unwound": 0, "alloc_bytes": 0})
            agg["files"] += 1
            agg["phases"].update(run["phases"])
            for k, v in run["sites"].items():
                a = agg["sites"].setdefault(k, {"phase": v["phase"], "calls": 0, "self": 0.0, "self_raw": 0.0,
                                                "incl_raw": 0.0})
                for f in ("calls", "self", "self_raw", "incl_raw"):
                    a[f] += v[f]
            agg["total"] += run["total"]
            agg["total_raw"] += run["total_raw"]
            agg["frames"] += run["frames"]
            agg["ev_user"] += run["user"]
            agg["cal"].append(run["cal"])
            agg["unwound"] += run["unwound"]
            agg["alloc_bytes"] += run["alloc_bytes"]
            t = off_time(offdir, w, m.group("name"), r) if offdir else None
            if t:
                agg["off_wall"] += t[0]
                agg["off_user"] += t[1]
            else:
                agg["off_missing"] += 1
    return per


def med(xs):
    return statistics.median(xs) if xs else float("nan")


def fmt_range(xs, pct=True, digits=1):
    if not xs:
        return "n/a"
    f = (lambda x: "%.*f%%" % (digits, 100 * x)) if pct else (lambda x: "%.*f" % (digits, x))
    return "%s (%s-%s)" % (f(med(xs)), f(min(xs)), f(max(xs)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rundir")
    ap.add_argument("--off")
    ap.add_argument("--cal", default="median", choices=["min", "median"])
    ap.add_argument("--json")
    ap.add_argument("--md")
    ap.add_argument("--top", type=int, default=30)
    a = ap.parse_args()
    per = summarize(a.rundir, a.off, a.cal)
    res = {"cal": a.cal, "workloads": {}}
    lines = ["# cetta-internals P: evaluator phase shares (generated by bench/p/evprof_report.py, calibration=%s)"
             % a.cal, ""]
    lines.append("Shares of the overhead-corrected run time, summed over each workload's files; median over runs "
                 "(min-max). Time is exclusive: each tick belongs to the innermost timed function.")
    lines.append("")
    # overhead table
    lines.append("## Instrumentation overhead")
    lines.append("")
    lines.append("| | " + " | ".join(WORKLOADS) + " |")
    lines.append("|---|" + "---|" * len(WORKLOADS))
    rows = collections.OrderedDict()
    for w in WORKLOADS:
        runs = per.get(w, {})
        rw = res["workloads"].setdefault(w, {})
        rw["runs"] = sorted(runs)
        ratios_raw = [x["total_raw"] / x["off_wall"] for x in runs.values() if x["off_wall"] and not x["off_missing"]]
        ratios_corr = [x["total"] / x["off_wall"] for x in runs.values() if x["off_wall"] and not x["off_missing"]]
        user_ratio = [x["ev_user"] / x["off_user"] for x in runs.values() if x["off_user"] and not x["off_missing"]]
        rows.setdefault("files x runs", []).append("%d x %d" % (max([x["files"] for x in runs.values()] or [0]), len(runs)))
        rows.setdefault("timed frames per run (M)", []).append("%.1f" % (med([x["frames"] for x in runs.values()]) / 1e6))
        cin = [c[0] for x in runs.values() for c in x["cal"]]
        cout = [c[1] for x in runs.values() for c in x["cal"]]
        rows.setdefault("c_in / c_out per frame (ticks, median over files)", []).append("%.1f / %.1f" % (med(cin), med(cout)))
        rows.setdefault("timers-on wall / timers-off wall, raw", []).append(fmt_range(ratios_raw, pct=False, digits=3))
        rows.setdefault("timers-on wall corrected / timers-off wall (residual)", []).append(fmt_range(ratios_corr, pct=False, digits=3))
        rows.setdefault("timers-on user / timers-off user", []).append(fmt_range(user_ratio, pct=False, digits=3))
        rows.setdefault("frames left by non-local exits (unwound)", []).append(str(sum(x["unwound"] for x in runs.values())))
        rw["overhead"] = {"raw": ratios_raw, "corrected": ratios_corr, "user": user_ratio, "c_in": med(cin),
                          "c_out": med(cout), "frames_median": med([x["frames"] for x in runs.values()])}
    for k, v in rows.items():
        lines.append("| %s | %s |" % (k, " | ".join(v)))
    lines.append("")
    # grouped phases
    for title, keyset, labeler in (("## The order's phases (grouped)", GROUP_ORDER, None),
                                   ("## Phases", PHASE_ORDER, PHASE_LABEL)):
        lines.append(title)
        lines.append("")
        lines.append("| phase | " + " | ".join(WORKLOADS) + " |")
        lines.append("|---|" + "---|" * len(WORKLOADS))
        for k in keyset:
            cells = []
            for w in WORKLOADS:
                runs = per.get(w, {})
                shares = []
                for x in runs.values():
                    if labeler:
                        v = x["phases"].get(k, 0.0)
                    else:
                        v = sum(val for ph, val in x["phases"].items() if GROUP.get(ph) == k)
                    shares.append(v / x["total"])
                key = "phase_shares" if labeler else "group_shares"
                res["workloads"].setdefault(w, {}).setdefault(key, {})[k] = shares
                cells.append(fmt_range(shares))
            lab = "%s (%s)" % (k, labeler[k]) if labeler else k
            lines.append("| %s | %s |" % (lab, " | ".join(cells)))
        lines.append("")
    # seconds
    lines.append("Corrected run time per workload (s, median over runs): " + ", ".join(
        "%s %.1f" % (w, med([x["total"] for x in per.get(w, {}).values()])) for w in WORKLOADS))
    lines.append("")
    # sites
    lines.append("## Top timed functions by corrected self time (share of the workload's corrected total)")
    lines.append("")
    allsites = collections.Counter()
    for w in WORKLOADS:
        for x in per.get(w, {}).values():
            for k, v in x["sites"].items():
                allsites[k] += v["self"] / x["total"]
    lines.append("| function | phase | " + " | ".join("%s self | calls/run" % w for w in WORKLOADS) + " |")
    lines.append("|---|---|" + "---|---|" * len(WORKLOADS))
    for k, _ in allsites.most_common(a.top):
        cells, phase = [], ""
        for w in WORKLOADS:
            runs = list(per.get(w, {}).values())
            if not runs or k not in runs[0]["sites"]:
                cells += ["", ""]
                continue
            phase = runs[0]["sites"][k]["phase"]
            cells.append("%.2f%%" % (100 * med([x["sites"][k]["self"] / x["total"] for x in runs])))
            cells.append("%.3gM" % (med([x["sites"][k]["calls"] for x in runs]) / 1e6))
        lines.append("| %s | %s | %s |" % (k, phase, " | ".join(cells)))
    for w in WORKLOADS:
        runs = list(per.get(w, {}).values())
        res["workloads"].setdefault(w, {})["sites"] = {
            k: {"phase": runs[0]["sites"][k]["phase"],
                "self_share": med([x["sites"][k]["self"] / x["total"] for x in runs]),
                "calls": med([x["sites"][k]["calls"] for x in runs]),
                "incl_raw_share": med([x["sites"][k]["incl_raw"] / x["total_raw"] for x in runs])}
            for k in runs[0]["sites"]} if runs else {}
        res["workloads"][w]["total_s"] = med([x["total"] for x in runs]) if runs else None
        res["workloads"][w]["alloc_bytes"] = med([x["alloc_bytes"] for x in runs]) if runs else None
    text = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(text)
    else:
        print(text)
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1, sort_keys=True)


if __name__ == "__main__":
    main()
