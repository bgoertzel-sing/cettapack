#!/usr/bin/env bash
# S1 step 3: w1-w3 end to end, packs off and on, interleaved run by run, plus S0's base build as a reference arm.
#   base  build/s0/cetta-base           (a9b3a811 + 0001-0004; S0's base)
#   off   build/s1/cetta-sites, CETTA_S1_SITES=0  (the compiled site linked in, switched off)
#   on    build/s1/cetta-sites, CETTA_S1_SITES=1  (packs on)
# Round r runs every workload in each arm, the arm order rotating with r (base off on, off on base, on base off, ...)
# so no arm always runs first.  Per run: bench/s1/run_workloads.sh's files and check line.
#   RUNS=5 bench/s1/run_e2e.sh [OUT]       # default evidence/s1-2026-11-24/e2e
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$ROOT/evidence/s1-2026-11-24/e2e}
RUNS=${RUNS:-5}
ARMS=(base off on)
for r in $(seq 1 "$RUNS"); do
  for i in 0 1 2; do
    v=${ARMS[$(( (r - 1 + i) % 3 ))]}
    RUN_FIRST=$r RUNS=1 "$ROOT/bench/s1/run_workloads.sh" "$v" "$OUT/$v"
  done
done
