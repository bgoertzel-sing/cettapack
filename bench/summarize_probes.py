#!/usr/bin/env python3
"""Summarize bench/run_probes.sh JSONL for one site: one table per probe.

Per-query time: each process reports the median of its REPEAT batches; the
tables give the median over processes and the min-max range over processes.
Paired ratios divide run i of the reference mode by run i of the other mode
(they ran back to back); > 1 means the other mode is faster.
"""
import json
import statistics
import sys
from collections import defaultdict

rows = defaultdict(list)
for line in open(sys.argv[1]):
    line = line.strip()
    if line.startswith("{"):
        r = json.loads(line)
        rows[r["mode"]].append(r)
site = rows["generic"][0]["site"] if "generic" in rows else next(iter(rows.values()))[0]["site"]


def vals(mode, key):
    return [r[key] for r in sorted(rows[mode], key=lambda r: r["run"]) if r.get(key) is not None]


def paired(ref, other):
    a = {r["run"]: r["per_query_us_median"] for r in rows[ref]}
    b = {r["run"]: r["per_query_us_median"] for r in rows[other]}
    rs = [a[k] / b[k] for k in sorted(a) if k in b and b[k]]
    if not rs:
        return None
    return statistics.median(rs), min(rs), max(rs), sum(x > 1 for x in rs), len(rs)


def route(mode):
    if "route_before" not in rows[mode][0]:
        return "not recorded (cettapack-host bench)"
    b = {(r["route_before"]["backend"], r["route_before"]["native_rows"], r["route_before"]["dense_facts"],
          r["route_after"]["native_rows"], r["route_after"]["dense_facts"]) for r in rows[mode]}
    return "; ".join(f"{k[0]}, rows+dense {k[1]:,}+{k[2]:,} -> {k[3]:,}+{k[4]:,}" for k in sorted(b))


def path(mode):
    r = rows[mode]
    calls = sum(x.get("site_calls", x.get("calls", 0)) for x in r)
    k = sum(x.get("kernel_calls", 0) for x in r)
    f = sum(x.get("fallbacks", 0) for x in r)
    fc = sum(x.get("fn_calls", 0) for x in r)
    fs = sum(x.get("fn_spec_calls", 0) for x in r)
    ff = sum(x.get("fn_fallbacks", 0) for x in r)
    parts = []
    if calls:
        parts.append(f"site calls {calls:,}, kernel {k:,}, fallback {f:,}")
    if fc:
        parts.append(f"fnspec calls {fc:,}, specialised {fs:,}, residual {ff:,}")
    return "; ".join(parts) if parts else "-"


def table(title, modes, ref, note=None):
    modes = [m for m in modes if m in rows]
    print(f"### {title}\n")
    print("| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs " + ref + ": paired median (range, faster runs) | path (all runs) |")
    print("|---|---|---|---|---|---|---|---|")
    for m in modes:
        p = paired(ref, m) if m != ref else None
        ps = f"{p[0]:.2f}x ({p[1]:.2f}-{p[2]:.2f}x, {p[3]}/{p[4]})" if p else "-"
        rss = vals(m, "peak_rss_kb")
        t = vals(m, "per_query_us_median")
        ans = sorted(set(vals(m, "answers_per_batch")))
        print(f"| {m} | {statistics.median(t):,.1f} | {min(t):,.1f}-{max(t):,.1f} | {statistics.median(rss) / 1024:.1f} | "
              f"{min(rss) / 1024:.1f}-{max(rss) / 1024:.1f} | {','.join(f'{a:,}' for a in ans)} | {ps} | {path(m)} |")
    print()
    ans = {m: tuple(sorted(set(vals(m, "answers_per_batch")))) for m in modes}
    if len(set(ans.values())) > 1:
        print(f"- WARNING: answers per batch differ between modes: {ans}\n")
    if note:
        print(note + "\n")
    print("Route (match backend and native storage before -> after the timed batches):\n")
    for m in modes:
        print(f"- {m}: {route(m)}")
    print()


print(f"## {site}\n")
print(f"{len(rows.get('generic', []))} processes per mode, interleaved run by run.\n")
table("Probe 1: direct entry point (match-rows, upstream/cetta/0003)", ["generic", "hosted-generic", "direct", "kernel", "fallback", "m-generic", "m-hosted-generic"], "hosted-generic",
      "m-generic / m-hosted-generic: the same modes measured by cettapack-host bench, the docs/BENCHMARK.md method (query atoms built inside the timed loop; generic parses the whole match text per query).")
table("Probe 2: fusing several matches (nested matches vs one fused kernel)", ["nested", "generic", "hosted-generic", "kernel", "fallback"], "hosted-generic",
      "nested: the same matches written one inside the other, run by CeTTa; kernel: the compiled kernel of the fused conjunction.")
table("Probe 3: specialising a whole function", ["fn-generic", "fn-hosted", "fn-kernel", "fn-spec", "fn-fallback"], "fn-hosted",
      "fn-hosted is hosted-generic for a function: the site callable on CeTTa's matcher inside the MeTTa function.")
