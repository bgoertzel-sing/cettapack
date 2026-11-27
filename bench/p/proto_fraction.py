#!/usr/bin/env python3
"""cetta-internals P: the prototype's measured fraction of the four lookup/refusal functions it indexes (C2 support).

  proto_fraction.py PROTO_SITES.json OUT.json

fraction = (seconds removed in the group's functions) / (their seconds without 0012), per workload (medians over
runs, bench/p/proto_sites.py); the estimate uses the smallest of the three (conservative).
"""
import json
import sys

# the four functions the index changes: the refusal check, the invocation plan and try (which hold the inlined
# positive lookup walk), and the refusal query of the PeTTa machine
group = ["prepared_pure_program_cache_has_decline_head", "prepared_pure_closed_invocation_plan",
         "petta_eval_machine_prepared_refused", "prepared_pure_closed_invocation_try"]
d = json.load(open(sys.argv[1]))
per = {}
for w, x in d.items():
    base = sum(x["sites"][s][0] for s in group if s in x["sites"])
    new = sum(x["sites"][s][1] for s in group if s in x["sites"])
    per[w] = (base - new) / base if base else 0.0
f = min(per.values())
json.dump({"functions": group, "fraction_removed": per, "min": f}, open(sys.argv[2], "w"), indent=1)
print(per, f)
