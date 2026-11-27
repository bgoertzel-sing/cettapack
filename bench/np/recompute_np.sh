#!/usr/bin/env bash
# Native primitives: recompute every generated table from the raw evidence and compare byte for byte with the
# committed files.  Needs the prof build (build/np/cetta-prof, for addr2line on the raw samples; its sha256 must be the
# one in evidence/np-2026-11-26/binaries.txt) and the PeTTaChainer checkout (the image the proof-store rows are read
# from).  Exit 1 on any difference.
#   bash bench/np/recompute_np.sh
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
E=evidence/np-2026-11-26
T=$(mktemp -d)
fail=0
say() { echo "$1: $2"; [ "$2" = identical ] || fail=1; }
# 1. Do 1: samples -> symbolised -> per-row breakdown
want=$(grep ' build/np/cetta-prof/cetta$' $E/binaries.txt | cut -d' ' -f1)
have=$(sha256sum build/np/cetta-prof/cetta 2>/dev/null | cut -d' ' -f1)
if [ "$want" != "$have" ]; then
  say "do1/breakdown (prof binary sha256 $have, recorded $want)" DIFFERS
else
  IMG=$T/image
  mkdir -p $IMG
  SCRATCH=${S0_SCRATCH:-$ROOT/../../scratch}
  git -C "${CHAINER_REPO:-$SCRATCH/survey/PeTTaChainer}" archive b0e24f9b9d7106ccabf51917f1703abf3ab8c570 pettachainer/metta \
    | tar -x -C $IMG --strip-components=2
  mkdir -p $T/sym
  for f in $E/do1/*/*.samples.txt.gz; do
    w=$(basename $(dirname $f)); b=$(basename $f .samples.txt.gz)
    python3 bench/s1/analyze_samples.py symbolise $f ${f%.samples.txt.gz}.fnprof.json --binary build/np/cetta-prof/cetta \
      --out $T/sym/$w.$b.sym.json >/dev/null
  done
  python3 bench/np/row_breakdown.py --runs $E/do1 --sym $T/sym --image $IMG --json $T/breakdown.json > $T/breakdown.md
  sed -i "s#$T/sym#build/np/sym#" $T/breakdown.md
  cmp -s $T/breakdown.md $E/do1/breakdown.md && say do1/breakdown.md identical || say do1/breakdown.md DIFFERS
  cmp -s $T/breakdown.json $E/do1/breakdown.json && say do1/breakdown.json identical || say do1/breakdown.json DIFFERS
fi
# 2. per-run check tables
for d in do1 check bench bench-aa bench-offon bench-off; do
  [ -d $E/$d ] || continue
  python3 bench/np/summarize_checks.py $E/$d --check >/dev/null && say $d/checks.md identical || say $d/checks.md DIFFERS
done
# 3. bag tests
python3 bench/np/summarize_bagtests.py $E/bagtest --check >/dev/null && say bagtest/summary.md identical || say bagtest/summary.md DIFFERS
# 4. paired tables
python3 bench/np/summarize_bench.py $E/bench --base on-none --new on-on --check >/dev/null && say bench/summary identical || say bench/summary DIFFERS
python3 bench/np/summarize_bench.py $E/bench-aa --base on-none --new on-off --check >/dev/null && say bench-aa/summary identical || say bench-aa/summary DIFFERS
python3 bench/np/summarize_bench.py $E/bench-offon --base on-off --new on-on --check >/dev/null && say bench-offon/summary identical || say bench-offon/summary DIFFERS
python3 bench/np/summarize_bench.py $E/bench-off --base off-none --new off-on --check >/dev/null && say bench-off/summary identical || say bench-off/summary DIFFERS
rm -rf $T
exit $fail
