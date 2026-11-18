#!/usr/bin/env python3
"""Summarize route-probe outputs: one row per (build, site, mode).
  summarize_probe.py DIR [DIR...]  (each DIR from run_probe.sh)
Route keys (CeTTa runtime counters over the timed batches, per query):
  nq   native-query-admission  (cetta_native_query_run admitted: the native query engine)
  nql  bindings-loop-call-native-query
  mnp  match-native-probe      (native backend candidate matcher)
  pil  match-position-index-lookup
  pm   sum of pathmap-* and mork-* counters (PathMap/MORK route); listed by name when nonzero
"""
import glob, json, os, re, sys
KEYS = [("nq", "native-query-admission"), ("nqa", "native-query-answer"), ("nql", "bindings-loop-call-native-query"),
        ("mnp", "match-native-probe"), ("mnc", "match-native-candidates"), ("pil", "match-position-index-lookup"),
        ("smset", "match-smset-rows"), ("unif", "unification-attempt")]
rows = []
for d in sys.argv[1:]:
    for f in sorted(glob.glob(os.path.join(d, "*.probe"))):
        base = os.path.basename(f)[:-6]
        label, mode = base.split("-", 1)
        txt = open(f).read()
        phases = {}
        for m in re.finditer(r"route-probe (\{.*\})", txt):
            j = json.loads(m.group(1)); phases[j["phase"]] = j
        ctr = {}
        for m in re.finditer(r"route-probe counter \S+ (\S+) (\d+)", txt):
            ctr[m.group(1)] = int(m.group(2))
        stats_on = "build_with_stats=1" in txt
        res = json.load(open(os.path.join(d, base + ".json")))
        nq = res["queries"] * res["repeat"]
        pm = {k: v for k, v in ctr.items() if k.startswith("pathmap-") or k.startswith("mork-")}
        rows.append(dict(dir=os.path.basename(d.rstrip("/")), site=label, mode=mode,
                         backend=phases.get("before-timed", {}).get("backend"),
                         rows_before=phases.get("before-timed", {}).get("native_rows"),
                         dense_before=phases.get("before-timed", {}).get("dense_facts"),
                         shadow=phases.get("before-timed", {}).get("pathmap_native_shadow_synced"),
                         env=phases.get("before-timed", {}).get("env_CETTA_PATHMAP_QUERY_INDEX"),
                         forced_wo_rows=phases.get("before-timed", {}).get("index_forced_without_rows"),
                         answers=res["answers_per_batch"], calls=res["calls"], kernel_calls=res["kernel_calls"],
                         fallbacks=res["fallbacks"], last_reason=res["last_reason"], stats=stats_on,
                         per_q={k: round(ctr.get(n, 0) / nq, 3) for k, n in KEYS} if stats_on else None,
                         pathmap=pm if stats_on else None))
print("| build | site | mode | backend | native rows + dense facts (before timed) | PathMap shadow | CETTA_PATHMAP_QUERY_INDEX | forced w/o rows | answers/batch | kernel calls / fallbacks | per query: native-query adm. / loop-call-native-query / match-native-probe / pos-index lookup | PathMap/MORK counters |")
print("|---|---|---|---|---|---|---|---|---|---|---|---|")
for r in rows:
    pq = r["per_q"]
    pqs = "%s / %s / %s / %s" % (pq["nq"], pq["nql"], pq["mnp"], pq["pil"]) if pq else "(stats off)"
    pm = ("none" if not r["pathmap"] else ", ".join("%s=%d" % kv for kv in sorted(r["pathmap"].items()))) if r["stats"] else "(stats off)"
    print("| %s | %s | %s | %s | %s + %s | %s | %s | %s | %d | %d / %d | %s | %s |" % (
        r["dir"], r["site"], r["mode"], r["backend"], r["rows_before"], r["dense_before"], r["shadow"],
        r["env"] or "unset", {1: "yes", 0: "no", -1: "n/a (no such check)"}.get(r["forced_wo_rows"]), r["answers"],
        r["kernel_calls"], r["fallbacks"], pqs, pm))
json.dump(rows, open(os.path.join(sys.argv[1], "..", "routes-" + "-".join(os.path.basename(d.rstrip("/")) for d in sys.argv[1:]) + ".json"), "w"), indent=1)
