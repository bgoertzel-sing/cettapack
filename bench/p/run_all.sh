#!/usr/bin/env bash
# cetta-internals P: the profile runs, RUNS rounds (default 3); in each round every configuration runs w1-w3 once, the
# configurations in alternating order (forward in odd rounds, reversed in even ones) so host drift spreads evenly.
#   RUNS=3 bench/p/run_all.sh OUTDIR [CONFIG ...]
# CONFIG: off | ev | stats | samp | ablpp (off build, CETTA_PREPARED_PURE_REFERENCE=1) | abloe (off build,
#         CETTA_OPEN_EQUATIONS_REFERENCE=1); default: off ev
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?outdir}
shift
CONFIGS=("$@")
[ ${#CONFIGS[@]} -eq 0 ] && CONFIGS=(off ev)
RUNS=${RUNS:-3}
run_config() {
  local c=$1 r=$2
  case "$c" in
    off|ev|stats|samp) BUILD=$c RUN_FIRST=$r RUNS=1 P_KEEP_IMAGE= "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    ablpp) BUILD=off P_TAG=ablpp P_ENV="CETTA_PREPARED_PURE_REFERENCE=1" P_IMAGE="$ROOT/build/p/image-off-ablpp" \
             RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
    abloe) BUILD=off P_TAG=abloe P_ENV="CETTA_OPEN_EQUATIONS_REFERENCE=1" P_IMAGE="$ROOT/build/p/image-off-abloe" \
             RUN_FIRST=$r RUNS=1 "$ROOT/bench/p/run_workloads.sh" "$OUT/$c" ;;
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
