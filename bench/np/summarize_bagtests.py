#!/usr/bin/env python3
"""Heap bag tests: the summary table of bench/np/run_bagtests.sh, recomputed from its output files.

  summarize_bagtests.py DIR            # writes DIR/summary.md (exit 1 if any test fails)
  summarize_bagtests.py DIR --check    # recomputes and compares byte for byte

Per test program: cases printed by the reference run (none) out of cases in the program; exit codes of none/off/on/
check; stdout of off, on and check byte-identical to none; check mode: heap_pop calls compared / differing; on mode:
heap_pop answered natively / heap calls that ran in SWI (heap_pop declines plus calls CeTTa's own native heap
declined).  PASS needs every exit 0, all cases printed, all stdout identical, 0 differing, and native > 0.
"""
import glob
import json
import os
import sys


def stat(d, t, m):
    try:
        ops = json.load(open(os.path.join(d, "%s.%s.np-heaps.json" % (t, m))))["ops"]
    except (OSError, ValueError):
        return None
    return {k: sum(o[k] for o in ops) for k in ("calls", "native", "declined", "checked", "mismatched")}


def build(d):
    lines = ["| test | cases | none exit | off = none | on = none | check = none | check: checked / mismatched "
             "| on: native / declined | verdict |", "|---|---|---|---|---|---|---|---|---|"]
    fail = False
    for prog in sorted(glob.glob(os.path.join(d, "h*.metta"))):
        t = os.path.basename(prog)[:-len(".metta")]
        out = {m: open(os.path.join(d, "%s.%s.stdout" % (t, m)), "rb").read() for m in ("none", "off", "on", "check")}
        ex = "/".join(open(os.path.join(d, "%s.%s.exit" % (t, m))).read().strip() for m in ("none", "off", "on", "check"))
        cases = out["none"].decode().count("\n(np-result ") + out["none"].decode().startswith("(np-result ")
        ncases = sum(1 for line in open(prog) if line.startswith("!(let*"))
        same = {m: out[m] == out["none"] for m in ("off", "on", "check")}
        ck, on = stat(d, t, "check"), stat(d, t, "on")
        ok = (ex == "0/0/0/0" and cases == ncases and all(same.values()) and ck and ck["mismatched"] == 0
              and on and on["native"] > 0)
        fail |= not ok
        yn = lambda b: "yes" if b else "NO"  # noqa: E731
        lines.append("| %s | %d of %d printed | %s | %s | %s | %s | %s | %s | %s |" % (
            t, cases, ncases, ex, yn(same["off"]), yn(same["on"]), yn(same["check"]),
            "%d / %d" % (ck["checked"], ck["mismatched"]) if ck else "MISSING",
            "%d / %d" % (on["native"], on["declined"]) if on else "MISSING", "PASS" if ok else "FAIL"))
    return "\n".join(lines) + "\n", fail


def main():
    d = sys.argv[1]
    md, fail = build(d)
    p = os.path.join(d, "summary.md")
    if "--check" in sys.argv:
        same = open(p).read() == md
        print("identical" if same else "DIFFERS")
        sys.exit(0 if same else 1)
    open(p, "w").write(md)
    print(md, end="")
    sys.exit(1 if fail else 0)


if __name__ == "__main__":
    main()
