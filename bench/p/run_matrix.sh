#!/usr/bin/env bash
# cetta-internals P: the quiet measurement matrix.  RUNS rounds; in each round every configuration runs w1-w3 once,
# in forward order in odd rounds and reversed in even rounds (host drift spreads evenly).  Nothing else of this task
# runs meanwhile (builds and symbolisation finished first), so the end-to-end ratios compare like with like.
#   RUNS=3 MIMALLOC=/path/libmimalloc.so bench/p/run_matrix.sh OUTDIR [CONFIG ...]
# CONFIG (directory OUTDIR/CONFIG, files NAME.TAG.rN.*):
#   off       build/p/cetta-off (timers compiled out; the base of every ratio)                 TAG off
#   ev        build/p/cetta-ev (timers on)                                                      TAG ev
#   proto     build/p/cetta-off-proto (off + upstream/cetta-p-proto/0012, the calibration prototype) TAG off-proto
#   evproto   build/p/cetta-ev-proto (ev + 0012: the prototype's effect per timed function)      TAG ev-proto
#   ablpp     off build, CETTA_PREPARED_PURE_REFERENCE=1 (prepared-pure call path refuses every call)  TAG off-ablpp
#   abloe     off build, CETTA_OPEN_EQUATIONS_REFERENCE=1 (no open-equation machine)          TAG off-abloe
#   mimalloc  off build, LD_PRELOAD=$MIMALLOC (every malloc/free of the process to mimalloc)  TAG off-mimalloc
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?outdir}
shift
CONFIGS=("$@")
[ ${#CONFIGS[@]} -eq 0 ] && CONFIGS=(off ev proto evproto ablpp abloe mimalloc)
RUNS=${RUNS:-3}
B=$ROOT/build/p
run_config() {
  local c=$1 r=$2
  case "$c" in
    off) BUILD=off RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    ev) BUILD=ev RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    proto) BUILD=off P_TAG=proto P_BUILD_DIR=$B/cetta-off-proto RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    evproto) BUILD=ev P_TAG=proto P_BUILD_DIR=$B/cetta-ev-proto RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    ablpp) BUILD=off P_TAG=ablpp P_ENV="CETTA_PREPARED_PURE_REFERENCE=1" RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    abloe) BUILD=off P_TAG=abloe P_ENV="CETTA_OPEN_EQUATIONS_REFERENCE=1" RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    mimalloc) BUILD=off P_TAG=mimalloc P_ENV="LD_PRELOAD=${MIMALLOC:?set MIMALLOC}" RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    *) echo "unknown config $c" >&2; exit 2 ;;
  esac
}
for r in $(seq 1 "$RUNS"); do
  order=("${CONFIGS[@]}")
  if [ $((r % 2)) -eq 0 ]; then
    order=(); for ((i=${#CONFIGS[@]}-1; i>=0; i--)); do order+=("${CONFIGS[$i]}"); done
  fi
  for c in "${order[@]}"; do run_config "$c" "$r"; done
done
