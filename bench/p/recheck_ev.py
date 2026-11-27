#!/usr/bin/env python3
"""cetta-internals P: the per-run route check of the timer runs, recomputed (sanity gate).

  recheck_ev.py RUNDIR...    # rewrites WORKLOAD/NAME.ev*.rN.check from the .stdout, .exit and .evprof.json

The first runner's route test compared the report's top_incl_ticks with total_ticks; top_incl_ticks also counts the
exit-time calibration frames (which run after the last stamp), so short files "failed" by the calibration's own
time (0.1-7%).  The corrected test uses the sum of the sites' self ticks, which is the time inside timed frames:
ok when the report parses, frames > 0, no stack overflow, no frame unwound by a non-local exit, and the time inside
frames is at most the total.  The answer part of the check line (exit, stdout sha256, same as S0, failed tests) is
recomputed exactly as the runner computes it.
"""
import glob
import hashlib
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, "evidence", "s0-2026-11-24", "runs", "base")
for rundir in sys.argv[1:]:
    for check in sorted(glob.glob(os.path.join(rundir, "*", "*.ev*.r*.check"))):
        w = os.path.basename(os.path.dirname(check))
        stem = check[:-len(".check")]
        name = re.match(r"^(.+)\.ev[^.]*\.r\d+$", os.path.basename(stem)).group(1)
        out = open(stem + ".stdout", "rb").read()
        sha = hashlib.sha256(out).hexdigest()
        ref = os.path.join(REF, w, name + ".r1.stdout")
        same = "same" if os.path.exists(ref) and hashlib.sha256(open(ref, "rb").read()).hexdigest() == sha else "DIFFERENT"
        fails = out.decode("utf-8", "replace").count("\u274c")
        ex = open(stem + ".exit").read().strip()
        try:
            d = json.load(open(stem + ".evprof.json"))
            inside = sum(s["self"] for s in d["sites"])
            n = sum(s["calls"] for s in d["sites"])
            ok = d["format"] == "cetta-evprof-v1" and n > 0 and d["overflow"] == 0 and d["unwound"] == 0 and \
                inside <= d["total_ticks"]
            route = "%s frames=%d covered=%.3f overflow=%d unwound=%d" % ("ok" if ok else "FAILED", n,
                                                                       inside / d["total_ticks"], d["overflow"],
                                                                       d["unwound"])
        except Exception:
            route = "MISSING"
        line = "exit=%s stdout=%s answers_vs_s0_base=%s failed_tests=%d route=%s" % (ex, sha, same, fails, route)
        old = open(check).read().strip()
        if old != line:
            open(check + ".orig", "w").write(old + "\n")
            open(check, "w").write(line + "\n")
        print(os.path.relpath(check, rundir), line)
