#!/usr/bin/env python3
"""S1: check a bag test's site counters against bench/s1/bagtest/expected.json.
  check_routes.py EXPECTED TEST ON_STATS INVALID_STATS   -> prints a one-cell summary; exit 1 on mismatch"""
import json
import sys


def counters(path):
    d = json.load(open(path))
    s = d["sites"][0]
    c = {k: s[k] for k in ("offered", "recognised", "specialised", "answers", "rows", "revalidations")}
    for k, v in s["residual"].items():
        c["residual." + k] = v
    c["residual"] = sum(s["residual"].values())
    return c


def main():
    exp = json.load(open(sys.argv[1]))[sys.argv[2]]
    on, inv = counters(sys.argv[3]), counters(sys.argv[4])
    bad = []
    for k, v in exp.get("exact", {}).items():
        if on.get(k) != v:
            bad.append("%s=%s (want %s)" % (k, on.get(k), v))
    for k, v in exp.get("min", {}).items():
        if on.get(k, 0) < v:
            bad.append("%s=%s (want >=%s)" % (k, on.get(k), v))
    if on["specialised"] + on["residual"] != on["recognised"]:
        bad.append("specialised+residual != recognised")
    if inv["specialised"] != 0 or inv["residual.forced"] != inv["recognised"] or inv["recognised"] != on["recognised"]:
        bad.append("invalid mode: spec=%d forced=%d recognised=%d" % (inv["specialised"], inv["residual.forced"],
                                                                      inv["recognised"]))
    res = {k: v for k, v in on.items() if k.startswith("residual.") and v}
    print("recognised %d, specialised %d, residual %d %s | forced %d/%d%s" % (
        on["recognised"], on["specialised"], on["residual"], json.dumps(res) if res else "",
        inv["residual.forced"], inv["recognised"], (" | MISMATCH: " + "; ".join(bad)) if bad else ""), end="")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
