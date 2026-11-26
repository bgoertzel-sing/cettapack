#!/usr/bin/env bash
# S2: sampler runs of the fnsprof build, packs off and on (poff, pon), RUNS rounds alternating the arm order, each
# round under the shared bench lock (/tmp/cettapack-bench.lock) with load average logged; raw samples gzipped.
# Tables: bench/s2/compiled_samples.py OUT.
#   RUNS=1 bench/s2/run_sprof.sh OUT
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?out}
mkdir -p "$OUT"
RUNS=${RUNS:-1}
for r in $(seq 1 "$RUNS"); do
  if [ $((r % 2)) -eq 1 ]; then order="poff pon"; else order="pon poff"; fi
  flock "${BENCH_LOCK:-/tmp/cettapack-bench.lock}" env ROOT="$ROOT" OUT="$OUT" R="$r" ORDER="$order" bash -c '
    set -euo pipefail
    start=$(date -u +%FT%TZ); l0=$(cut -d" " -f1-3 /proc/loadavg)
    for v in $ORDER; do RUN_FIRST=$R RUNS=1 "$ROOT/bench/s2/run_workloads.sh" "$v" "$OUT/$v"; done
    echo "round=$R order=${ORDER// /,} start=$start end=$(date -u +%FT%TZ) load_start=$l0 load_end=$(cut -d" " -f1-3 /proc/loadavg)" >> "$OUT/batches.txt"
  '
done
gzip -f "$OUT"/*/*/*.samples.txt
