#!/usr/bin/env bash
# S2 bag tests for the compiled functions (bench/s2/bagtest/t*.metta, each prefixed by common.inc), on the S2 build
# (build/s2/cetta-fns) in three modes, each in its own process: off (CETTA_S2_FNS=0: CeTTa's own answers), on (=1)
# and invalid (every guard forced invalid).  Per test: stdout and exit of on and invalid must equal off byte for
# byte, and the counters ($CETTA_S2_FNS_STATS) must show the routes bench/s2/bagtest/expected.json states
# (bench/s2/check_routes.py).  Results: OUT/<test>.<mode>.{stdout,stderr,exit,stats.json}, OUT/summary.md;
# exit 1 on any failure.
#   bench/s2/run_bagtests.sh [OUT]
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?OUT}; shift
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
CETTA=${CETTA:-$ROOT/build/s2/cetta-fns/cetta}
SCRATCH=${S0_SCRATCH:-$ROOT/../../scratch}
CHAINER_REPO=${CHAINER_REPO:-$SCRATCH/survey/PeTTaChainer}
PETTA_REPO=${PETTA_REPO:-$SCRATCH/survey/PeTTa}
IMG=$ROOT/build/s2/image-bagtest
rm -rf "$IMG"; mkdir -p "$IMG"
git -C "$CHAINER_REPO" archive b0e24f9b9d7106ccabf51917f1703abf3ab8c570 pettachainer/metta | tar -x -C "$IMG" --strip-components=2
git -C "$PETTA_REPO" archive e038e4dbb587e48fdb9d14990966108d38fde0b3 lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
cp "$CETTA" "$IMG/cetta"
{ echo "cetta=$CETTA sha256=$(sha256sum "$CETTA" | cut -d' ' -f1)"; echo "build_head=$(cat "$(dirname "$CETTA")/.s2-build-head" 2>/dev/null)"; echo "head=$(git -C "$ROOT" rev-parse HEAD) dirty=$(git -C "$ROOT" status --porcelain --untracked-files=no | wc -l) date=$(date -u +%FT%TZ)"; } > "$OUT/env.txt"
fail=0
echo "| test | off = on | off = invalid | routes | verdict |" > "$OUT/summary.md"
echo "|---|---|---|---|---|" >> "$OUT/summary.md"
TESTS=("$@")
[ ${#TESTS[@]} -eq 0 ] && TESTS=("$ROOT"/bench/s2/bagtest/t*.metta)
for t in "${TESTS[@]}"; do
  n=$(basename "$t" .metta)
  cat "$ROOT/bench/s2/bagtest/common.inc" "$t" > "$IMG/examples/s2_bag_$n.metta"
  for m in off on invalid; do
    v=0; [ $m = on ] && v=1; [ $m = invalid ] && v=invalid
    ( cd "$IMG" && env CETTA_PETTA_SEARCH_MACHINE=1 CETTA_S1_SITES=0 CETTA_S2_FNS=$v CETTA_S2_FNS_STATS="$OUT/$n.$m.stats.json" \
        CETTA_S2_FNS_TRACE=${TRACE:-0} timeout 600 ./cetta --lang petta --profile extended --space-engine native "examples/s2_bag_$n.metta" \
        > "$OUT/$n.$m.stdout" 2> "$OUT/$n.$m.stderr" )
    echo $? > "$OUT/$n.$m.exit"
  done
  eq_on=$(cmp -s "$OUT/$n.off.stdout" "$OUT/$n.on.stdout" && cmp -s "$OUT/$n.off.exit" "$OUT/$n.on.exit" && echo yes || echo NO)
  eq_inv=$(cmp -s "$OUT/$n.off.stdout" "$OUT/$n.invalid.stdout" && cmp -s "$OUT/$n.off.exit" "$OUT/$n.invalid.exit" && echo yes || echo NO)
  route=$(python3 "$ROOT/bench/s2/check_routes.py" "$ROOT/bench/s2/bagtest/expected.json" "$n" "$OUT/$n.on.stats.json" "$OUT/$n.invalid.stats.json")
  rc=$?
  verdict=pass
  if [ "$eq_on" != yes ] || [ "$eq_inv" != yes ] || [ $rc -ne 0 ]; then verdict=FAIL; fail=1; fi
  echo "| $n | $eq_on | $eq_inv | $route | $verdict |" >> "$OUT/summary.md"
  echo "$n off=on:$eq_on off=invalid:$eq_inv $route $verdict"
done
exit $fail
