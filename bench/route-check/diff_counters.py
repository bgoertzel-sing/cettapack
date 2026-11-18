#!/usr/bin/env python3
"""Per-query CeTTa runtime counters of one mode, two builds, side by side.
  diff_counters.py A.probe A.json B.probe B.json"""
import json, re, sys
def load(p, j):
    res = json.load(open(j)); n = res["queries"] * res["repeat"]
    c = {m.group(1): int(m.group(2)) for m in re.finditer(r"route-probe counter \S+ (\S+) (\d+)", open(p).read())}
    return c, n
a, na = load(sys.argv[1], sys.argv[2]); b, nb = load(sys.argv[3], sys.argv[4])
print("| counter | per query (first) | per query (second) |")
print("|---|---|---|")
for k in sorted(set(a) | set(b), key=lambda k: -(abs(a.get(k, 0) / na - b.get(k, 0) / nb))):
    x, y = a.get(k, 0) / na, b.get(k, 0) / nb
    if abs(x - y) > 1e-9:
        print("| %s | %.2f | %.2f |" % (k, x, y))
print("\nsame per-query value in both: " + ", ".join("%s=%.2f" % (k, a[k] / na) for k in sorted(set(a) & set(b)) if abs(a[k] / na - b[k] / nb) < 1e-9))
