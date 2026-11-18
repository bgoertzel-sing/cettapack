#!/usr/bin/env bash
# Run the route probe for every mode and site of one probe build.
#   bench/route-check/run_probe.sh <probe dir> <a9|4c> <out dir> [REPEAT]
# Inputs: the benchmark's own KB and query files (copied from wt/m1-numbers/build,
# generated there by bench/gen_kb.py with the benchmark's arguments).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
P=$1; V=$2; OUT=$3; REPEAT=${4:-1}
mkdir -p "$OUT"
if [ "$V" = a9 ]; then
  SUITES="main:target_expressed gene_variants:gene_variants tissue_fanout:tissue_fanout variant_interactor:variant_interactor"
  MODES="generic hosted-generic kernel kernel-cursor fallback"
else
  SUITES="main:target_expressed"; MODES="generic hosted-generic kernel fallback"
fi
for su in $SUITES; do
  label=${su%%:*}; site=${su#*:}
  for m in $MODES; do
    cmd="$P/cettapack-host-$site bench --kb build/rc/inputs/kb-$label.metta --queries build/rc/inputs/queries-$label.txt --mode $m --repeat $REPEAT"
    echo "CPK_ROUTE_PROBE=1 $cmd" >> "$OUT/commands.txt"
    CPK_ROUTE_PROBE=1 $cmd > "$OUT/$label-$m.json" 2> "$OUT/$label-$m.probe"
  done
done
echo done
