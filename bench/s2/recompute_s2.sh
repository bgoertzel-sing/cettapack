#!/usr/bin/env bash
# S2: recompute every generated table from the raw evidence and compare byte for byte with the committed files.
# Exit 1 on any difference.
#   bash bench/s2/recompute_s2.sh
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
E=evidence/s2-2026-11-26
T=$(mktemp -d)
fail=0
say() { echo "$1: $2"; [ "$2" = identical ] || fail=1; }
# 1. end-to-end tables (time, memory, paired ratios, noise verdict, function counters)
python3 bench/s2/summarize_e2e.py $E/e2e --check >/dev/null && say e2e/summary identical || say e2e/summary DIFFERS
# 2. coverage estimate (S1 profile x S2 counters)
python3 bench/s2/coverage.py --s1-prof evidence/s1-2026-11-24/prof-pilot --s1-step4 evidence/s1-2026-11-24/step4/step4.json \
  --e2e $E/e2e --out-md $E/coverage/coverage.md --out-json $E/coverage/coverage.json --check >/dev/null \
  && say coverage identical || say coverage DIFFERS
# 3. coverage measured from the sampler (fnsprof build, on arm)
if [ -f $E/sprof/samples_compiled.md ]; then
  python3 bench/s2/compiled_samples.py $E/sprof --check >/dev/null && say sprof/samples_compiled identical || say sprof/samples_compiled DIFFERS
fi
# 4. bag-test route verdicts
for t in bench/s2/bagtest/t*.metta; do
  n=$(basename $t .metta)
  python3 bench/s2/check_routes.py bench/s2/bagtest/expected.json $n $E/bagtest/$n.on.stats.json $E/bagtest/$n.invalid.stats.json > $T/r 2>&1
  rc=$?
  eq=$(cmp -s $E/bagtest/$n.off.stdout $E/bagtest/$n.on.stdout && cmp -s $E/bagtest/$n.off.stdout $E/bagtest/$n.invalid.stdout && echo yes || echo no)
  grep -q "| $n | yes | yes | $(cat $T/r) | pass |" $E/bagtest/summary.md && [ $rc -eq 0 ] && [ $eq = yes ] \
    && say bagtest/$n identical || say bagtest/$n DIFFERS
done
rm -rf $T
exit $fail
