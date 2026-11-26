#!/usr/bin/env bash
# Native primitives, Do 4: paired, interleaved end-to-end runs of w1-w3.
#
#   BUILD=on RUNS=5 bench/np/run_bench.sh OUTDIR [WORKLOAD ...]
#
# For run r = 1..RUNS, one batch: every workload, base (SHIM=none: PeTTaChainer exactly as S0 ran it) and np
# (SHIM=on: the native heaps), in alternating order (odd r: base first; even r: np first), the same binary
# (build/np/cetta-$BUILD) and the same image for both.  Each batch holds the shared-host lock
# (flock /tmp/cettapack-bench.lock) and releases it between batches; the load average at its start and end goes to
# OUTDIR/ledger.txt.  BASE_SHIM / NEW_SHIM override the two arms (e.g. none/none for a base-vs-base spread).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?outdir}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
shift 1
WORKLOADS=("$@")
[ ${#WORKLOADS[@]} -eq 0 ] && WORKLOADS=(w1-examples w2-wmtm w3-benchgen)
RUNS=${RUNS:-5}
RUN_FIRST=${RUN_FIRST:-1}
BUILD=${BUILD:-on}
BASE_SHIM=${BASE_SHIM:-none}
NEW_SHIM=${NEW_SHIM:-on}
LOCK=${BENCH_LOCK:-/tmp/cettapack-bench.lock}
export NP_IMAGE=$ROOT/build/np/image-bench-$BUILD
rm -rf "$NP_IMAGE"
for r in $(seq "$RUN_FIRST" $((RUN_FIRST + RUNS - 1))); do
  if [ $((r % 2)) -eq 1 ]; then order=("$BASE_SHIM" "$NEW_SHIM"); else order=("$NEW_SHIM" "$BASE_SHIM"); fi
  flock "$LOCK" bash -c '
    set -euo pipefail
    echo "batch r='"$r"' order='"${order[*]}"' start=$(date -u +%FT%TZ) load_start=$(cut -d" " -f1-3 /proc/loadavg)" >> "'"$OUT"'/ledger.txt"
    for w in '"${WORKLOADS[*]}"'; do
      for s in '"${order[*]}"'; do
        BUILD='"$BUILD"' SHIM=$s RUN_FIRST='"$r"' RUNS=1 NP_KEEP_IMAGE=1 "'"$ROOT"'/bench/np/run_workloads.sh" "'"$OUT"'" "$w"
      done
    done
    echo "batch r='"$r"' end=$(date -u +%FT%TZ) load_end=$(cut -d" " -f1-3 /proc/loadavg)" >> "'"$OUT"'/ledger.txt"
  '
done
