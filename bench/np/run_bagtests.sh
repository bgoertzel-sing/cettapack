#!/usr/bin/env bash
# Heap bag tests (native primitives): bench/np/gen_heap_bagtests.py's programs, each run in its own process on one
# np build in four modes: none (no shim: SWI-Prolog's library(heaps), the reference), off (shim switch off), on
# (native heaps), check (both per call, SWI answers, differences counted).  Pass: exit 0 in every mode, stdout of
# off/on/check byte-identical to none, check mode 0 mismatched calls, on mode answered natively (native > 0) with the
# declines reported.  Writes OUTDIR/<test>.<mode>.{stdout,stderr,exit,np-heaps.json} and OUTDIR/summary.md.
#
#   BUILD=on bench/np/run_bagtests.sh OUTDIR
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:?outdir}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
BUILD=${BUILD:-on}
BDIR=${NP_BUILD_DIR:-$ROOT/build/np/cetta-$BUILD}
SCRATCH=${S0_SCRATCH:-$ROOT/../../scratch}
CHAINER_REPO=${CHAINER_REPO:-$SCRATCH/survey/PeTTaChainer}
PETTA_REPO=${PETTA_REPO:-$SCRATCH/survey/PeTTa}
IMG=$ROOT/build/np/image-bagtest
rm -rf "$IMG"; mkdir -p "$IMG"
git -C "$CHAINER_REPO" archive b0e24f9b9d7106ccabf51917f1703abf3ab8c570 pettachainer/metta | tar -x -C "$IMG" --strip-components=2
git -C "$PETTA_REPO" archive e038e4dbb587e48fdb9d14990966108d38fde0b3 lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
cp "$BDIR/cetta" "$IMG/cetta"
cp "$ROOT"/bench/np/shim/np_heaps*.metta "$IMG/"
mkdir -p "$IMG/bagtest"
TESTS=$(python3 "$ROOT/bench/np/gen_heap_bagtests.py" "$IMG/bagtest" --seed "${SEED:-20261126}")
{
  echo "cetta=$BDIR/cetta sha256=$(sha256sum "$BDIR/cetta" | cut -d' ' -f1) patches=$(tr '\n' ' ' < "$BDIR/.np-build-patches")"
  echo "cettapack_head=$(git -C "$ROOT" rev-parse HEAD) seed=${SEED:-20261126} date=$(date -u +%FT%TZ)"
  for t in $TESTS; do echo "test $t sha256=$(sha256sum "$IMG/bagtest/$t.metta" | cut -d' ' -f1)"; done
} > "$OUT/env.txt"
cp "$IMG"/bagtest/*.metta "$OUT/"
echo "| test | cases | none exit | off = none | on = none | check = none | check: checked / mismatched | on: native / declined | verdict |" > "$OUT/summary.md"
echo "|---|---|---|---|---|---|---|---|---|" >> "$OUT/summary.md"
fail=0
for t in $TESTS; do
  for m in none off on check; do
    args=(--lang petta --profile extended --space-engine native)
    case $m in off) args+=(np_heaps_off.metta) ;; on) args+=(np_heaps.metta) ;; check) args+=(np_heaps_check.metta) ;; esac
    set +e
    ( cd "$IMG" && env CETTA_PETTA_SEARCH_MACHINE=1 CETTA_NP_HEAPS_STATS="$OUT/$t.$m.np-heaps.json" \
        CETTA_NP_HEAPS_LOG="$OUT/$t.$m.np-heaps.log" timeout 1800 ./cetta "${args[@]}" "bagtest/$t.metta" \
        > "$OUT/$t.$m.stdout" 2> "$OUT/$t.$m.stderr" )
    echo $? > "$OUT/$t.$m.exit"
    set -e
  done
  read -r cases < <(grep -c '^(np-result ' "$OUT/$t.none.stdout" || true)
  ncases=$(grep -c '^!(let\*' "$IMG/bagtest/$t.metta")
  same() { cmp -s "$OUT/$t.none.stdout" "$OUT/$t.$1.stdout" && echo yes || echo NO; }
  stat() { python3 -c 'import json,sys
d=json.load(open(sys.argv[1]));o=d["ops"]
print(" / ".join(str(sum(x[k] for x in o)) for k in sys.argv[2:]))' "$OUT/$t.$1.np-heaps.json" "${@:2}" 2>/dev/null || echo MISSING; }
  ex="$(cat "$OUT/$t.none.exit")/$(cat "$OUT/$t.off.exit")/$(cat "$OUT/$t.on.exit")/$(cat "$OUT/$t.check.exit")"
  so=$(same off); sn=$(same on); sc=$(same check)
  ck=$(stat check checked mismatched); nt=$(stat on native declined)
  v=PASS
  [ "$ex" = "0/0/0/0" ] || v=FAIL
  [ "$cases" = "$ncases" ] || v=FAIL
  [ "$so$sn$sc" = yesyesyes ] || v=FAIL
  case "$ck" in *" / 0") ;; *) v=FAIL ;; esac
  case "$nt" in "0 / "*|MISSING) v=FAIL ;; esac
  [ "$v" = PASS ] || fail=1
  echo "| $t | $cases of $ncases printed | $ex | $so | $sn | $sc | $ck | $nt | $v |" >> "$OUT/summary.md"
done
cat "$OUT/summary.md"
exit $fail
