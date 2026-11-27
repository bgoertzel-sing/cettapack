#!/usr/bin/env bash
# cetta-internals P: symbolise the sampler runs (bench/s1/analyze_samples.py symbolise) and compute the sampler's
# phase shares per workload (bench/p/sample_phases.py), gzipping the raw sample files afterwards.
#   bench/p/sample_report.sh RUNDIR OUTDIR     # RUNDIR = .../samp (WORKLOAD/NAME.samp.rN.samples.txt[.gz])
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RUN=${1:?rundir}
OUT=${2:?outdir}
SYM=${P_SYM_DIR:-$ROOT/build/p/sym}
mkdir -p "$OUT" "$SYM"
for w in w1-examples w2-wmtm w3-benchgen; do
  for f in "$RUN/$w"/*.samples.txt "$RUN/$w"/*.samples.txt.gz; do
    [ -e "$f" ] || continue
    b=$(basename "$f"); b=${b%.gz}; b=${b%.samples.txt}
    s="$SYM/$w.$b.sym.json"
    [ -s "$s" ] || python3 "$ROOT/bench/s1/analyze_samples.py" symbolise "$f" "$RUN/$w/$b.fnprof.json" --out "$s"
    case "$f" in *.gz) ;; *) gzip -f "$f" ;; esac
  done
  python3 "$ROOT/bench/p/sample_phases.py" "$ROOT/bench/p/evprof_sites.txt" "$SYM/$w".*.sym.json --label "$w" \
    --top 25 --json "$OUT/sample_phases.$w.json" > "$OUT/sample_phases.$w.txt"
done
