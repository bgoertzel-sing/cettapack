#!/usr/bin/env bash
# Next-step probes (t-2026-11-18-cettapack-next-step-probe): one suite per site,
# every mode of build/cettapack-probe-<site> interleaved run by run, as
# bench/run_bench.sh does: RUNS processes per mode, REPEAT batches of all
# seeds per process, the process's median batch. Same KB and seeds as
# docs/BENCHMARK.md (bench/gen_kb.py --seed 20261106, --query-kind per site).
#
#   RUNS=7 REPEAT=5 OUT=evidence/probes-2026-11-18 bench/run_probes.sh
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
RUNS=${RUNS:-7}
REPEAT=${REPEAT:-5}
OUT=${OUT:-evidence/probes}
SUITES=${SUITES:-"target_expressed gene_variants tissue_fanout variant_interactor"}
# m-generic / m-hosted-generic: build/cettapack-host-<site> bench, the docs/BENCHMARK.md method
# (query atoms built inside the timed loop: generic parses the match text per query,
# the callable modes parse only the seed), interleaved with the probe modes
MODES=${MODES:-"generic nested direct hosted-generic kernel fallback fn-generic fn-hosted fn-kernel fn-spec fn-fallback m-generic m-hosted-generic"}
mkdir -p "$OUT" build
{
  echo "date: $(date -u +%FT%TZ)"
  echo "host: $(hostname) $(uname -srm); cpus=$(nproc); $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')"
  echo "mem: $(free -g | awk '/Mem:/ {print $2" GiB"}')"
  echo "cettapack: $(git rev-parse HEAD) ($(git status --porcelain --untracked-files=no | wc -l) modified tracked paths)"
  echo "cetta: $(git -C cetta rev-parse HEAD) + upstream/cetta patches: $(ls upstream/cetta/*.patch | xargs -n1 basename | tr '\n' ' ')"
  echo "cetta build tree head: $(git -C build/cetta-src log -1 --format='%h %s')"
  echo "gcc: $(gcc -dumpversion); python: $(python3 --version 2>&1)"
  echo "RUNS=$RUNS REPEAT=$REPEAT SUITES=$SUITES"
  echo "MODES=$MODES"
  echo "load average at start: $(cut -d' ' -f1-3 /proc/loadavg)"
} > "$OUT/env.txt"
: > "$OUT/commands.txt"
for site in $SUITES; do
  case $site in
    target_expressed) args=(--query-kind rs); label=main ;;
    gene_variants) args=(--query-kind gene); label=gene_variants ;;
    tissue_fanout) args=(--query-kind tissue --queries 40); label=tissue_fanout ;;
    variant_interactor) args=(--query-kind rs); label=variant_interactor ;;
  esac
  kb=build/kb-$label.metta q=build/queries-$label.txt
  python3 bench/gen_kb.py "${args[@]}" --out "$kb" --queries-out "$q" | tee "$OUT/kb-$site.txt"
  : > "$OUT/runs-$site.jsonl"
  for r in $(seq 1 "$RUNS"); do
    for mode in $MODES; do
      case $mode in
        m-*) cmd="build/cettapack-host-$site bench --kb $kb --queries $q --mode ${mode#m-} --repeat $REPEAT" ;;
        *) cmd="build/cettapack-probe-$site bench --kb $kb --queries $q --mode $mode --repeat $REPEAT" ;;
      esac
      echo "[$site] $cmd" >> "$OUT/commands.txt"
      if [ "${mode#m-}" != "$mode" ]; then
        $cmd | sed "s/^{\"mode\":\"/{\"run\":$r,\"site\":\"$site\",\"mode\":\"m-/" | tee -a "$OUT/runs-$site.jsonl"
      else
        $cmd | sed "s/^{/{\"run\":$r,/" | tee -a "$OUT/runs-$site.jsonl"
      fi
    done
  done
  echo "load average at end of $site: $(cut -d' ' -f1-3 /proc/loadavg)" >> "$OUT/env.txt"
  python3 bench/summarize_probes.py "$OUT/runs-$site.jsonl" | tee "$OUT/summary-$site.md"
done
