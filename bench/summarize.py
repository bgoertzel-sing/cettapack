#!/usr/bin/env python3
"""Summarize bench JSONL: per mode, medians across processes plus spread.

Per-query time: each process reports the median of its REPEAT batches; the
table gives the median over processes and the min-max range over processes.
Peak RSS: median and range of the processes' ru_maxrss.
"""
import json
import statistics
import sys
from collections import defaultdict

rows = defaultdict(list)
order = []
for line in open(sys.argv[1]):
    line = line.strip()
    if line.startswith("{"):
        r = json.loads(line)
        if r["mode"] not in rows:
            order.append(r["mode"])
        rows[r["mode"]].append(r)

PREFERRED = ["generic", "hosted-generic", "kernel", "kernel-cursor", "fallback",
             "m1-generic", "m1-hosted-generic", "m1-kernel", "m1-fallback", "he"]
modes = [m for m in PREFERRED if m in rows] + [m for m in order if m not in PREFERRED]


def vals(mode, key):
    return [r[key] for r in rows[mode] if r.get(key) is not None]


def med(mode, key):
    v = vals(mode, key)
    return statistics.median(v) if v else None


def fmt(x, nd=1):
    if x is None:
        return "-"
    return f"{x:.{nd}f}" if isinstance(x, float) else str(x)


def spread(mode, key, scale=1.0, nd=1):
    v = vals(mode, key)
    if not v:
        return "-"
    return f"{min(v) / scale:.{nd}f}-{max(v) / scale:.{nd}f}"


cols = ["mode", "processes", "queries/batch", "answers/batch", "per-query us (median)", "per-query us (range)",
        "peak RSS MiB (median)", "peak RSS MiB (range)", "guard ns/call", "narrow ns/call", "rematch ns/call",
        "answer ns/call", "fallback ns/call", "fallback freq", "candidates/call", "index build (ms)"]
print("| " + " | ".join(cols) + " |")
print("|" + "---|" * len(cols))
for mode in modes:
    rss = med(mode, "peak_rss_kb")
    print("| " + " | ".join([
        mode, str(len(rows[mode])), fmt(med(mode, "queries")), fmt(med(mode, "answers_per_batch")),
        fmt(med(mode, "per_query_us_median"), 2), spread(mode, "per_query_us_median", 1.0, 2),
        fmt(rss / 1024 if rss else None), spread(mode, "peak_rss_kb", 1024.0, 1),
        fmt(med(mode, "guard_ns_per_call")), fmt(med(mode, "narrow_ns_per_call")),
        fmt(med(mode, "rematch_ns_per_call")), fmt(med(mode, "answer_ns_per_call")),
        fmt(med(mode, "fallback_ns_per_call")), fmt(med(mode, "fallback_frequency"), 3),
        fmt(med(mode, "candidates_per_call"), 3), fmt(med(mode, "index_build_ms")),
    ]) + " |")
print()
ans = {m: med(m, "answers_per_batch") for m in modes if m != "he"}
if len(set(ans.values())) > 1:
    print(f"- WARNING: answers per batch differ between modes: {ans}")
def paired(base, other):
    """generic/other per interleaved run (run i of each mode ran back to back)"""
    a, b = vals(base, "per_query_us_median"), vals(other, "per_query_us_median")
    rs = [x / y for x, y in zip(a, b) if y]
    if not rs:
        return ""
    return f"; paired by run: median {statistics.median(rs):.2f}x, range {min(rs):.2f}-{max(rs):.2f}x, faster in {sum(r > 1 for r in rs)}/{len(rs)} runs"


g = med("generic", "per_query_us_median")
for m in ("kernel", "kernel-cursor", "hosted-generic", "fallback"):
    x = med(m, "per_query_us_median")
    if g and x:
        print(f"- {m} vs generic CeTTa (same pin), end to end: {g / x:.2f}x per query (>1 means faster than generic){paired('generic', m)}")
hg, kx = med("hosted-generic", "per_query_us_median"), med("kernel", "per_query_us_median")
if hg and kx:
    print(f"- kernel vs CeTTa's matcher behind the same callable (hosted-generic): {hg / kx:.2f}x{paired('hosted-generic', 'kernel')}")
if med("m1-generic", "per_query_us_median") and kx:
    print(f"- kernel (this branch) vs M1-pin generic CeTTa: {med('m1-generic', 'per_query_us_median') / kx:.2f}x{paired('m1-generic', 'kernel')}")
mg, mk = med("m1-generic", "per_query_us_median"), med("m1-kernel", "per_query_us_median")
if mg and mk:
    print(f"- M1 baseline rerun (old pin): kernel vs generic {mg / mk:.2f}x per query")
if mg and g:
    print(f"- generic CeTTa, new pin vs M1 pin: {mg / g:.2f}x per query (>1 means the new pin is faster)")
k = med("kernel", "per_query_us_median")
if mk and k:
    print(f"- kernel, this branch vs M1 kernel: {mk / k:.2f}x per query")
nb, nc = med("kernel", "narrow_ns_per_call"), med("kernel-cursor", "narrow_ns_per_call")
if nb and nc:
    print(f"- narrowing: bulk {nb:.0f} ns/call vs per-byte cursor {nc:.0f} ns/call ({nc / nb:.1f}x less)")
