#!/usr/bin/env python3
"""S2 bag tests: check a test's counters against bench/s2/bagtest/expected.json.

expected.json: {test: {"answered_min": {fn: n}, "invalid_min": {fn: n}, "declined_call_min": {fn: n},
                       "reasons": [substring, ...]}}
In invalid mode every offered call must be declined_forced (nothing answered).  Prints a one-line route summary.
"""
import json, sys

exp_all, test, on_path, inv_path = sys.argv[1:5]
exp = json.load(open(exp_all)).get(test, {})
on = json.load(open(on_path))
inv = json.load(open(inv_path))
bad = []
F = on["functions"]
for fn, n in exp.get("answered_min", {}).items():
    if F[fn]["answered"] < n:
        bad.append("%s answered %d < %d" % (fn, F[fn]["answered"], n))
for fn, n in exp.get("invalid_min", {}).items():
    if F[fn]["declined_invalid"] < n:
        bad.append("%s declined_invalid %d < %d" % (fn, F[fn]["declined_invalid"], n))
for fn, n in exp.get("declined_call_min", {}).items():
    if F[fn]["declined_call"] < n:
        bad.append("%s declined_call %d < %d" % (fn, F[fn]["declined_call"], n))
for r in exp.get("reasons", []):
    if not any(r in k for k in on["reasons"]):
        bad.append("no decline reason containing %r" % r)
for fn, v in inv["functions"].items():
    if v["answered"] or v["offered"] != v["declined_forced"]:
        bad.append("invalid mode: %s answered %d, offered %d, forced %d" % (fn, v["answered"], v["offered"], v["declined_forced"]))
tot = lambda k: sum(v[k] for v in F.values())
summary = "offered %d answered %d declined: invalid %d, call %d; invalid mode forced %d/%d" % (
    tot("offered"), tot("answered"), tot("declined_invalid"), tot("declined_call"),
    sum(v["declined_forced"] for v in inv["functions"].values()), sum(v["offered"] for v in inv["functions"].values()))
print(summary + ("" if not bad else " MISMATCH: " + "; ".join(bad)))
sys.exit(1 if bad else 0)
