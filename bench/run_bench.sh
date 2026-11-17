#!/usr/bin/env bash
# CettaPack benchmark: one suite per site, the modes interleaved run by run.
#
# Suites (all on the same 128,576-atom seeded KB, bench/gen_kb.py defaults):
#   main                M1's site (target_expressed), 200 variant seeds
#   gene_variants       option B: seed in the second field of target, 200 gene seeds
#   tissue_fanout       option B: seed in the second field of expressed, large fan-out, 40 tissue seeds
#   variant_interactor  option B: three-way join through interacts' second field, 200 variant seeds
#   he-scale            M1's HE comparison (1,264-atom KB); not in the default list
#
# Modes (per process, REPEAT batches of all seeds; RUNS processes per mode):
#   generic         generic CeTTa: (match <space> <conjunction> <template>) through the evaluator
#   hosted-generic  CeTTa's generic conjunction matcher behind the site's callable
#   kernel          the compiled kernel, guard on every call, bulk narrowing (upstream/cetta 0002)
#   kernel-cursor   the same kernel narrowing through M1's per-byte cursor calls
#   fallback        guard -> INVALID on every call (PathMap disabled) -> residual
#   m1-*            (main suite, when M1_HOST is set) M1's binary on M1's CeTTa pin,
#                   modes generic/hosted-generic/kernel/fallback: the M1 baseline rerun
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
RUNS=${RUNS:-7}
REPEAT=${REPEAT:-5}
OUT=${OUT:-evidence/bench}
HE_QUERIES=${HE_QUERIES:-40}
SUITES=${SUITES:-"main gene_variants tissue_fanout variant_interactor"}
M1_HOST=${M1_HOST:-}
mkdir -p "$OUT" build
{
  echo "date: $(date -u +%FT%TZ)"
  echo "host: $(hostname) $(uname -srm); cpus=$(nproc); $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
  echo "mem: $(free -g | awk '/Mem:/ {print $2" GiB"}')"
  echo "cettapack: $(git rev-parse HEAD) ($(git status --porcelain --untracked-files=no | wc -l) modified tracked paths)"
  echo "cetta: $(git -C cetta rev-parse HEAD) + upstream/cetta patches: $(ls upstream/cetta/*.patch | xargs -n1 basename | tr '\n' ' ')"
  echo "M1 baseline host: ${M1_HOST:-none}"
  if [ -n "$M1_HOST" ]; then
    echo "M1 baseline tree: $(git -C "$(dirname "$M1_HOST")/.." rev-parse HEAD), cetta $(git -C "$(dirname "$M1_HOST")/../cetta" rev-parse HEAD)"
  fi
  echo "gcc: $(gcc -dumpversion); python: $(python3 --version 2>&1)"
  echo "RUNS=$RUNS REPEAT=$REPEAT SUITES=$SUITES"
  echo "load average at start: $(cut -d' ' -f1-3 /proc/loadavg)"
} > "$OUT/env.txt"
: > "$OUT/commands.txt"

suite() {
  local label=$1 site=$2 modes=$3; shift 3
  local kb=build/kb-$label.metta q=build/queries-$label.txt
  python3 bench/gen_kb.py "$@" --out "$kb" --queries-out "$q" | tee "$OUT/kb-$label.txt"
  : > "$OUT/runs-$label.jsonl"
  for r in $(seq 1 "$RUNS"); do
    for mode in $modes; do
      case $mode in
        he) cmd="python3 bench/he_bench.py --kb $kb --queries $q --limit $HE_QUERIES" ;;
        m1-*) cmd="$M1_HOST bench --kb $kb --queries $q --mode ${mode#m1-} --repeat $REPEAT" ;;
        *) cmd="build/cettapack-host-$site bench --kb $kb --queries $q --mode $mode --repeat $REPEAT" ;;
      esac
      echo "[$label] $cmd" >> "$OUT/commands.txt"
      if [ "${mode#m1-}" != "$mode" ]; then
        $cmd | sed 's/"mode":"/"mode":"m1-/' | tee -a "$OUT/runs-$label.jsonl"
      else
        $cmd | tee -a "$OUT/runs-$label.jsonl"
      fi
    done
  done
  echo "load average at end of $label: $(cut -d' ' -f1-3 /proc/loadavg)" >> "$OUT/env.txt"
  python3 bench/summarize.py "$OUT/runs-$label.jsonl" | tee "$OUT/summary-$label.md"
}

MODES="generic hosted-generic kernel kernel-cursor fallback"
for s in $SUITES; do
  case $s in
    main)
      m="$MODES"
      if [ -n "$M1_HOST" ]; then m="$m m1-generic m1-hosted-generic m1-kernel m1-fallback"; fi
      suite main target_expressed "$m" ;;
    gene_variants) suite gene_variants gene_variants "$MODES" --query-kind gene ;;
    tissue_fanout) suite tissue_fanout tissue_fanout "$MODES" --query-kind tissue --queries 40 ;;
    variant_interactor) suite variant_interactor variant_interactor "$MODES" ;;
    he-scale)
      HE_MODES=""
      if python3 -c "import hyperon" 2>/dev/null; then HE_MODES=he; fi
      suite he-scale target_expressed "generic kernel $HE_MODES" --snps 240 --genes 60 --interactions 300 --annotations 200 --queries $HE_QUERIES ;;
  esac
done
