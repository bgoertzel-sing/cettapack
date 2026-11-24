#!/usr/bin/env bash
# S1: recompute every generated table from the raw evidence and compare byte for byte with the committed files.
# Needs the prof build (build/s1/cetta-prof, for addr2line on the raw samples) and the PeTTaChainer/PeTTa
# checkouts (to rebuild the image the module map is read from).  Exit 1 on any difference.
#   bash bench/s1/recompute_s1.sh            # compare
#   bash bench/s1/recompute_s1.sh --write    # (re)generate the step-4 tables (evidence/s1-2026-11-24/step4)
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
E=evidence/s1-2026-11-24
T=$(mktemp -d)
fail=0
say() { echo "$1: $2"; [ "$2" = identical ] || fail=1; }
# 1. end-to-end summary
python3 bench/s1/summarize_e2e.py $E/e2e --check >/dev/null && say e2e/summary identical || say e2e/summary DIFFERS
python3 bench/s1/sprof_table.py $E/sprof --check >/dev/null && say sprof/summary identical || say sprof/summary DIFFERS
python3 bench/s1/m5_estimate.py $E/step4/step4.json --out-md $T/m5.md >/dev/null
cmp -s $T/m5.md $E/step4/m5_estimate.md && say step4/m5_estimate.md identical || say step4/m5_estimate.md DIFFERS
# 2. finding 1 goal tables
for s in 100 1000; do
  python3 bench/s1/check_w2_goals.py $E/finding1/s0_w2_wmtm.steps$s.metta $E/finding1/steps$s/w2-wmtm/s0_w2_wmtm.r1.stdout > $T/g$s.md
  cmp -s $T/g$s.md $E/finding1/steps$s/goals.md && say finding1/steps$s identical || say finding1/steps$s DIFFERS
done
python3 bench/s1/check_w2_goals.py $E/finding1/s0_w2_wmtm.steps100.metta evidence/s0-2026-11-24/runs/*/w2-wmtm/*.stdout > $T/s0g.md
cmp -s $T/s0g.md $E/finding1/s0-runs-goals.md && say finding1/s0-runs identical || say finding1/s0-runs DIFFERS
# 3. samples -> symbolised -> step 4 and site reach tables
IMG=$T/image
mkdir -p $IMG
SCRATCH=${S0_SCRATCH:-$ROOT/../../scratch}
git -C "${CHAINER_REPO:-$SCRATCH/survey/PeTTaChainer}" archive b0e24f9b9d7106ccabf51917f1703abf3ab8c570 pettachainer/metta | tar -x -C $IMG --strip-components=2
git -C "${PETTA_REPO:-$SCRATCH/survey/PeTTa}" archive e038e4dbb587e48fdb9d14990966108d38fde0b3 lib | tar -x -C $IMG --wildcards 'lib/lib_*.metta'
for d in $E/prof-pilot $E/prof-r2; do
  [ -d $d ] || continue
  for f in $d/*/*.samples.txt.gz; do
    w=$(basename $(dirname $f)); b=$(basename $f .samples.txt.gz)
    python3 bench/s1/analyze_samples.py symbolise $f ${f%.samples.txt.gz}.fnprof.json --binary build/s1/cetta-prof/cetta \
      --out $T/$(basename $d).$w.$b.sym.json >/dev/null
  done
done
ARGS=()
FN=()
for w in w1-examples w2-wmtm w3-benchgen; do
  ARGS+=(--sym "$w=$T/*.$w.*.sym.json")
  FN+=(--fnprof "$w=$E/prof-*/$w/*.fnprof.json")
done
python3 bench/s1/step4_search.py --image $IMG "${ARGS[@]}" "${FN[@]}" --out-md $T/step4.md --out-json $T/step4.json >/dev/null
sed -i "s#$T/##g" $T/step4.md
if [ "${1:-}" = --write ]; then
  mkdir -p $E/step4
  cp $T/step4.md $E/step4/step4.md; cp $T/step4.json $E/step4/step4.json
  python3 bench/s1/site_reach.py --classification evidence/s0-2026-11-24/classification.json "${ARGS[@]}" \
    --out-md $E/step4/site_reach.md --out-json $E/step4/site_reach.json >/dev/null
  sed -i "s#$T/##g" $E/step4/step4.json
fi
cmp -s $T/step4.md $E/step4/step4.md && say step4/step4.md identical || say step4/step4.md DIFFERS
python3 bench/s1/site_reach.py --classification evidence/s0-2026-11-24/classification.json "${ARGS[@]}" --out-md $T/reach.md >/dev/null
cmp -s $T/reach.md $E/step4/site_reach.md && say step4/site_reach.md identical || say step4/site_reach.md DIFFERS
rm -rf $T
exit $fail
