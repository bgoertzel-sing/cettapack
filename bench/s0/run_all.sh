#!/usr/bin/env bash
# S0: every workload on the three builds, interleaved run by run (base, off, on, base, off, on, ...), so the
# overhead ratios compare runs made under the same machine load.
#
#   bench/s0/run_all.sh [OUT]      # default evidence/s0-2026-11-24/runs; RUNS (default 3)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$ROOT/evidence/s0-2026-11-24/runs}
RUNS=${RUNS:-3}
for r in $(seq 1 "$RUNS"); do
  for v in base off on; do
    RUN_FIRST=$r RUNS=1 "$ROOT/bench/s0/run_workloads.sh" "$v" "$OUT/$v"
  done
done
