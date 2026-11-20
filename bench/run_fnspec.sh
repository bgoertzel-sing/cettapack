#!/usr/bin/env bash
# fnspec benchmark (t-2026-11-18-cettapack-fnspec): the probes' method
# (docs/NEXT_STEP_PROBES.md): RUNS processes per mode, REPEAT batches of all
# queries per process, the process's median batch, modes interleaved run by run;
# every query atom built before the timed loop, one untimed warm-up call.
#
#   RUNS=7 REPEAT=5 OUT=evidence/fnspec-2026-11-18/bench bench/run_fnspec.sh [site ...]
#
# Default sites: every fnspec site with a benchmark KB (the num_* and imp_* sites are test-only).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
RUNS=${RUNS:-7}
REPEAT=${REPEAT:-5}
OUT=${OUT:-evidence/fnspec}
MODES=${MODES:-"fn-generic fn-hosted fn-spec fn-fallback"}
if [ $# -gt 0 ]; then SITES="$*"; else
  SITES=$(for f in fnspec/sites/*.metta; do s=$(basename "$f" .metta); case $s in num_*|imp_*) ;; *) echo "$s";; esac; done)
fi
mkdir -p "$OUT" build
{
  echo "date: $(date -u +%FT%TZ)"
  echo "host: $(hostname) $(uname -srm); cpus=$(nproc); $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
  echo "mem: $(free -g | awk '/Mem:/ {print $2" GiB"}')"
  echo "cettapack: $(git rev-parse HEAD) ($(git status --porcelain --untracked-files=no | wc -l) modified tracked paths)"
  echo "cetta: $(git -C cetta rev-parse HEAD) + upstream/cetta patches: $(ls upstream/cetta/*.patch | xargs -n1 basename | tr '\n' ' ')"
  echo "cetta build tree head: $(git -C build/cetta-src log -1 --format='%h %s')"
  echo "gcc: $(gcc -dumpversion); python: $(python3 --version 2>&1)"
  echo "RUNS=$RUNS REPEAT=$REPEAT MODES=$MODES"
  echo "SITES=$(echo $SITES | tr '\n' ' ')"
  echo "load average at start: $(cut -d' ' -f1-3 /proc/loadavg)"
} > "$OUT/env.txt"
: > "$OUT/commands.txt"
: > "$OUT/inputs.txt"
: > "$OUT/checks.txt"
for site in $SITES; do
  args=$(python3 bench/fnspec_inputs.py --site "$site" --build build)
  [ -n "$args" ] || { echo "no benchmark KB for $site" >&2; continue; }
  kbf=$(echo "$args" | awk '{print $2}')
  qf=$(echo "$args" | awk '{print $4}')
  echo "$site: $args; kb sha256 $(sha256sum "$kbf" | cut -c1-16); queries sha256 $(sha256sum "$qf" | cut -c1-16)" >> "$OUT/inputs.txt"
  : > "$OUT/runs-$site.jsonl"
  for r in $(seq 1 "$RUNS"); do
    for mode in $MODES; do
      cmd="build/cettapack-fnspec bench --site $site $args --mode $mode --repeat $REPEAT"
      echo "$cmd" >> "$OUT/commands.txt"
      $cmd | sed "s/^{/{\"run\":$r,/" >> "$OUT/runs-$site.jsonl"
    done
  done
  echo "load average at end of $site: $(cut -d' ' -f1-3 /proc/loadavg)" >> "$OUT/env.txt"
  # known-answer canary: the probe's shape on the probe sites must give the probes' recorded
  # answers per batch (docs/NEXT_STEP_PROBES.md); every site: the structural check
  case $site in
    te_stream_filter) exp="--expect-answers 1620" ;; gv_stream_filter) exp="--expect-answers 7268" ;;
    tf_stream_filter) exp="--expect-answers 173693" ;; vi_stream_filter) exp="--expect-answers 9643" ;; *) exp="" ;;
  esac
  python3 bench/check_fnspec_site.py "$OUT/runs-$site.jsonl" $exp | tee -a "$OUT/checks.txt"
  echo "done $site"
done
python3 bench/summarize_fnspec.py "$OUT" > "$OUT/summary.md"
echo "summary: $OUT/summary.md"
