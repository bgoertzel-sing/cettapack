#!/usr/bin/env bash
# S1: profiled runs of the sitesprof build (compiled site + CETTA_FNPROF=1), sites off and on, alternating run by run
# (off on, on off, ...): the compiled site's own row against CeTTa's generic match (bench/s1/sprof_table.py).
#   RUNS=3 bench/s1/run_sprof.sh [OUT]     # default evidence/s1-2026-11-24/sprof
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$ROOT/evidence/s1-2026-11-24/sprof}
RUNS=${RUNS:-3}
for r in $(seq 1 "$RUNS"); do
  if [ $((r % 2)) -eq 1 ]; then order="0 1"; else order="1 0"; fi
  for s in $order; do
    m=$([ $s = 1 ] && echo on || echo off)
    SPROF_SITES=$s RUN_FIRST=$r RUNS=1 "$ROOT/bench/s1/run_workloads.sh" sprof "$OUT/$m"
  done
done
