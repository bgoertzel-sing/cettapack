#!/usr/bin/env bash
# S2 step 3: w1-w3 end to end, packs off and on, interleaved run by run, plus S0's base build as a reference arm.
#   base  S0's base build (a9b3a811 + 0001-0004)
#   off   build/s2/cetta-fns, CETTA_S2_FNS=0  (the compiled functions linked in, switched off)
#   on    build/s2/cetta-fns, CETTA_S2_FNS=1  (packs on)
# Round r runs every workload in each arm, the arm order rotating with r (base off on, off on base, on base off, ...)
# so no arm always runs first.  Per run: bench/s2/run_workloads.sh's files and check line.
# Shared host: each round (one interleaved batch) holds the exclusive lock /tmp/cettapack-bench.lock (shared with
# the other cettapack worker's timed runs) and releases it between rounds; OUT/batches.txt records each round's
# lock wait, start/end time and load average at start and end.
#   RUNS=5 [RUN_FIRST=1] [ARMS="base off on"] bench/s2/run_e2e.sh OUT [WORKLOAD ...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?out}; shift
mkdir -p "$OUT"
RUNS=${RUNS:-5}
FIRST=${RUN_FIRST:-1}
LOCK=${BENCH_LOCK:-/tmp/cettapack-bench.lock}
read -r -a ARMS <<< "${ARMS:-base off on}"
N=${#ARMS[@]}
for r in $(seq "$FIRST" $((FIRST + RUNS - 1))); do
  order=()
  for i in $(seq 0 $((N - 1))); do order+=("${ARMS[$(( (r - 1 + i) % N ))]}"); done
  asked=$(date -u +%FT%TZ)
  flock "$LOCK" env ROOT="$ROOT" OUT="$OUT" R="$r" ASKED="$asked" ORDER="${order[*]}" bash -c '
    set -euo pipefail
    start=$(date -u +%FT%TZ); l0=$(cut -d" " -f1-3 /proc/loadavg)
    for v in $ORDER; do
      RUN_FIRST=$R RUNS=1 "$ROOT/bench/s2/run_workloads.sh" "$v" "$OUT/$v" "$@"
    done
    echo "round=$R order=${ORDER// /,} lock_asked=$ASKED start=$start end=$(date -u +%FT%TZ) load_start=$l0 load_end=$(cut -d" " -f1-3 /proc/loadavg)" >> "$OUT/batches.txt"
  ' _ "$@"
done
