#!/usr/bin/env bash
# cetta-internals P: zero-code calibration runs on the timers-off build (Do 3/4): CeTTa's own reference switches and an
# allocator swap, each interleaved with plain off runs, RUNS rounds (default 3), order alternating by round.
#   RUNS=3 MIMALLOC=/path/libmimalloc.so bench/p/run_ablations.sh OUTDIR [CONFIG ...]
# CONFIG: base    the off build as is
#         ablpp   CETTA_PREPARED_PURE_REFERENCE=1: the prepared-pure call path refuses every call (reference path)
#         abloe   CETTA_OPEN_EQUATIONS_REFERENCE=1: calls do not use the open-equation machine (reference path)
#         mimalloc  LD_PRELOAD=$MIMALLOC: every malloc/free of the process goes to mimalloc instead of glibc
# Answers must stay byte-identical to S0's (the per-run check line); a configuration that changes them is reported.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?outdir}
shift
CONFIGS=("$@")
[ ${#CONFIGS[@]} -eq 0 ] && CONFIGS=(base ablpp abloe mimalloc)
RUNS=${RUNS:-3}
run_config() {
  local c=$1 r=$2 envs=""
  case "$c" in
    base) envs="" ;;
    ablpp) envs="CETTA_PREPARED_PURE_REFERENCE=1" ;;
    abloe) envs="CETTA_OPEN_EQUATIONS_REFERENCE=1" ;;
    mimalloc) envs="LD_PRELOAD=${MIMALLOC:?set MIMALLOC}" ;;
    *) echo "unknown config $c" >&2; exit 2 ;;
  esac
  BUILD=off P_TAG=$c P_ENV="$envs" RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c"
}
for r in $(seq 1 "$RUNS"); do
  order=("${CONFIGS[@]}")
  if [ $((r % 2)) -eq 0 ]; then
    order=(); for ((i=${#CONFIGS[@]}-1; i>=0; i--)); do order+=("${CONFIGS[$i]}"); done
  fi
  for c in "${order[@]}"; do run_config "$c" "$r"; done
done
