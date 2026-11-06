#!/usr/bin/env bash
# CettaPack M1 benchmark (§17).
#
# Suite "main" (128,576-atom seeded KB, 200 seeds):
#   generic         generic CeTTa: (match <space> (, ...) template) through the evaluator
#   hosted-generic  CeTTa's generic conjunction matcher behind the same callable as the
#                   kernel (hosting overhead cancels against "kernel")
#   kernel          the compiled kernel, guard checks included on every call
#   fallback        guard -> INVALID on every call (PathMap disabled) -> residual
# Suite "he-scale" (1,264-atom KB, same generator and seed): generic, kernel and
#   HE, because HE 0.2.10 aborts (Rust panic in hyperon-space/src/index/trie.rs:179)
#   on this KB family from a few thousand atoms up (evidence/bench/he-crash.txt).
# PeTTa is not run: no SWI-Prolog on this host (docs/BENCHMARK.md).
#
# Each mode runs in RUNS separate processes (peak RSS is per process); each
# process times REPEAT batches of all seeds. Reported numbers are medians.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
RUNS=${RUNS:-5}
REPEAT=${REPEAT:-5}
OUT=${OUT:-evidence/bench}
HE_QUERIES=${HE_QUERIES:-40}
mkdir -p "$OUT" build
{
  echo "date: $(date -u +%FT%TZ)"
  echo "host: $(uname -srm); cpus=$(nproc); $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
  echo "mem: $(free -g | awk '/Mem:/ {print $2" GiB"}')"
  echo "cettapack: $(git rev-parse HEAD) ($(git status --porcelain --untracked-files=no | wc -l) modified tracked paths)"
  echo "cetta: $(git -C cetta rev-parse HEAD) + upstream/cetta patches"
  echo "gcc: $(gcc -dumpversion); python: $(python3 --version 2>&1)"
  echo "RUNS=$RUNS REPEAT=$REPEAT HE_QUERIES=$HE_QUERIES"
} > "$OUT/env.txt"
: > "$OUT/commands.txt"

suite() {
  local label=$1 modes=$2; shift 2
  local kb=build/kb-$label.metta q=build/queries-$label.txt
  python3 bench/gen_kb.py "$@" --out "$kb" --queries-out "$q" | tee "$OUT/kb-$label.txt"
  : > "$OUT/runs-$label.jsonl"
  for mode in $modes; do
    for r in $(seq 1 "$RUNS"); do
      if [ "$mode" = he ]; then
        cmd="python3 bench/he_bench.py --kb $kb --queries $q --limit $HE_QUERIES"
      else
        cmd="build/cettapack-host bench --kb $kb --queries $q --mode $mode --repeat $REPEAT"
      fi
      echo "[$label] $cmd" >> "$OUT/commands.txt"
      $cmd | tee -a "$OUT/runs-$label.jsonl"
    done
  done
  python3 bench/summarize.py "$OUT/runs-$label.jsonl" | tee "$OUT/summary-$label.md"
}

suite main "generic hosted-generic kernel fallback"
HE_MODES=""
python3 -c "import hyperon" 2>/dev/null && HE_MODES=he
suite he-scale "generic kernel $HE_MODES" --snps 240 --genes 60 --interactions 300 --annotations 200 --queries $HE_QUERIES
