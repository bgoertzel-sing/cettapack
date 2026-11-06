#!/usr/bin/env python3
"""Summarize bench JSONL: medians across processes for each mode."""
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


def med(mode, key):
    vals = [r[key] for r in rows[mode] if r.get(key) is not None]
    return statistics.median(vals) if vals else None


def fmt(x, nd=1):
    if x is None:
        return "-"
    return f"{x:.{nd}f}" if isinstance(x, float) else str(x)


cols = ["mode", "processes", "queries/batch", "answers/batch", "per-query median (us)",
        "guard ns/call", "narrow ns/call", "rematch ns/call", "answer ns/call",
        "fallback ns/call", "candidates/call", "rematches/call", "index build (ms)",
        "KB load (ms)", "peak RSS (MiB)"]
print("| " + " | ".join(cols) + " |")
print("|" + "---|" * len(cols))
for mode in ("generic", "hosted-generic", "kernel", "fallback", "he"):
    if not rows[mode]:
        continue
    rss = med(mode, "peak_rss_kb")
    print("| " + " | ".join([
        mode, str(len(rows[mode])), fmt(med(mode, "queries")), fmt(med(mode, "answers_per_batch")),
        fmt(med(mode, "per_query_us_median"), 2), fmt(med(mode, "guard_ns_per_call")),
        fmt(med(mode, "narrow_ns_per_call")), fmt(med(mode, "rematch_ns_per_call")),
        fmt(med(mode, "answer_ns_per_call")), fmt(med(mode, "fallback_ns_per_call")),
        fmt(med(mode, "candidates_per_call"), 3), fmt(med(mode, "rematches_per_call"), 3),
        fmt(med(mode, "index_build_ms")), fmt(med(mode, "kb_load_ms")),
        fmt(rss / 1024 if rss else None),
    ]) + " |")
g, k = med("generic", "per_query_us_median"), med("kernel", "per_query_us_median")
f, h = med("fallback", "per_query_us_median"), med("he", "per_query_us_median")
print()
hg = med("hosted-generic", "per_query_us_median")
if g and k:
    print(f"- kernel (guards included) vs generic CeTTa, end to end: {g / k:.2f}x per query (>1 means the kernel is faster)")
if hg and k:
    print(f"- kernel vs CeTTa's generic matcher behind the same callable: {hg / k:.2f}x per query")
kk, hk = med("kernel", "kernel_ns_per_call"), med("hosted-generic", "kernel_ns_per_call")
gk = med("kernel", "guard_ns_per_call")
if kk and hk:
    print(f"- in-callable work per call: kernel {kk / 1000:.2f} us + guard {gk / 1000:.2f} us vs generic matcher {hk / 1000:.2f} us")
if g and f:
    print(f"- forced fallback vs generic CeTTa: {f / g:.2f}x per query (guard + residual)")
if h and k:
    print(f"- kernel vs HE: {h / k:.1f}x per query (HE timed on its first {fmt(med('he', 'queries'))} seeds)")
