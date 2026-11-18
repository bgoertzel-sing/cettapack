#!/usr/bin/env bash
# Item 2: re-time plain match (generic) on M1's site on both builds, stats off,
# with the benchmarked binaries themselves, identical inputs/flags/answer
# consumption, interleaved run by run (RUNS=7 REPEAT=5 as before). Also
# hosted-generic (the same CeTTa matcher called from C, no evaluator around it)
# to split matcher time from evaluator time.
#   bench/route-check/retime.sh OUT
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
OUT=$1; RUNS=${RUNS:-7}; REPEAT=${REPEAT:-5}
M1=$ROOT/../m1-baseline/build/cettapack-host                 # d2df1cc, CeTTa 4c285a97 + M1 0001
A9=$ROOT/../m1-numbers/build/cettapack-host-target_expressed  # 4bd2687 code, CeTTa a9b3a811 + 0001 + 0002
KB=build/rc/inputs/kb-main.metta; Q=build/rc/inputs/queries-main.txt
mkdir -p "$OUT"
{
  echo "date: $(date -u +%FT%TZ)"; echo "host: $(hostname) $(uname -srm); $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
  echo "m1 binary: $M1 sha256 $(sha256sum "$M1" | cut -c1-16)"; echo "a9 binary: $A9 sha256 $(sha256sum "$A9" | cut -c1-16)"
  echo "kb sha256 $(sha256sum $KB | cut -c1-16); queries sha256 $(sha256sum $Q | cut -c1-16)"
  echo "env CETTA_*: $(env | grep '^CETTA_' | tr '\n' ' ')"; echo "RUNS=$RUNS REPEAT=$REPEAT ORDER=${ORDER:-m1 a9}"
  echo "load average at start: $(cut -d' ' -f1-3 /proc/loadavg)"
} > "$OUT/env.txt"
: > "$OUT/runs.jsonl"; : > "$OUT/commands.txt"
for r in $(seq 1 "$RUNS"); do
  ord=${ORDER:-m1 a9}
  if [ "$ord" = alternate ]; then if [ $((r % 2)) = 1 ]; then ord="m1 a9"; else ord="a9 m1"; fi; fi
  for b in $ord; do
    for m in generic hosted-generic; do
      bin=$M1; [ $b = a9 ] && bin=$A9
      cmd="$bin bench --kb $KB --queries $Q --mode $m --repeat $REPEAT"
      echo "$cmd" >> "$OUT/commands.txt"
      $cmd | sed "s/\"mode\":\"/\"run\":$r,\"build\":\"$b\",\"mode\":\"/" >> "$OUT/runs.jsonl"
    done
  done
done
echo "load average at end: $(cut -d' ' -f1-3 /proc/loadavg)" >> "$OUT/env.txt"
python3 - "$OUT/runs.jsonl" <<'EOF' | tee "$OUT/summary.md"
import json, sys, statistics as st
rows = [json.loads(l) for l in open(sys.argv[1])]
print("| build | mode | median us/query (min-max over runs) | answers/batch |")
print("|---|---|---|---|")
for b in ("m1", "a9"):
    for m in ("generic", "hosted-generic"):
        v = [r["per_query_us_median"] for r in rows if r["build"] == b and r["mode"] == m]
        a = {r["answers_per_batch"] for r in rows if r["build"] == b and r["mode"] == m}
        print("| %s | %s | %.1f (%.1f-%.1f) | %s |" % (b, m, st.median(v), min(v), max(v), sorted(a)))
for m in ("generic", "hosted-generic"):
    ratios = []
    for r in sorted({x["run"] for x in rows}):
        a = [x for x in rows if x["run"] == r and x["mode"] == m]
        d = {x["build"]: x["per_query_us_median"] for x in a}
        ratios.append(d["a9"] / d["m1"])
    print("%s: a9/m1 paired by run: median %.3f, min %.3f, max %.3f, a9 slower in %d/%d runs" % (
        m, st.median(ratios), min(ratios), max(ratios), sum(x > 1 for x in ratios), len(ratios)))
EOF
