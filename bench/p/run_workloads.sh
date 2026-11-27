#!/usr/bin/env bash
# cetta-internals P (t-2026-11-27-cetta-internals-p-profile): S0's workloads w1-w3 (the same PeTTaChainer b0e24f9b image
# with the PeTTa e038e4db lib overlay and the same generated programs as bench/s0 and bench/np) on the P builds.
#
#   BUILD=ev|off|stats|samp bench/p/run_workloads.sh OUTDIR [WORKLOAD ...]
#
# BUILD: build/p/cetta-$BUILD (bench/p/build_p_cetta.sh):
#   ev     phase timers on (CETTA_EVPROF_OUT=NAME.TAG.rN.evprof.json)
#   off    the same sources with the timers compiled out (the reference for overhead)
#   stats  CeTTa's own runtime counters (ENABLE_RUNTIME_STATS=1, --emit-runtime-stats; counters in NAME.TAG.rN.stderr)
#   samp   S1's C-level sampler (0005+0006; CETTA_FNPROF_OUT, CETTA_FNPROF_SAMPLES_OUT, 1 ms of CPU per sample)
# Per run: OUTDIR/WORKLOAD/NAME.TAG.rN.{stdout,stderr,time,exit,check}.  The check line (sanity gate, per unit): exit
# status, stdout sha256, whether it equals S0's recorded base stdout for the same file (the known answers), failed
# tests (❌), and for ev whether the timer report exists and covers the run (route).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:?ev|off|stats|samp}
OUT=${1:?outdir}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
shift 1
WORKLOADS=("$@")
[ ${#WORKLOADS[@]} -eq 0 ] && WORKLOADS=(w1-examples w2-wmtm w3-benchgen)
RUNS=${RUNS:-1}
RUN_FIRST=${RUN_FIRST:-1}
SCRATCH=${S0_SCRATCH:-$ROOT/../../scratch}
CHAINER_REPO=${CHAINER_REPO:-$SCRATCH/survey/PeTTaChainer}
PETTA_REPO=${PETTA_REPO:-$SCRATCH/survey/PeTTa}
CHAINER_REV=b0e24f9b9d7106ccabf51917f1703abf3ab8c570
PETTA_REV=e038e4dbb587e48fdb9d14990966108d38fde0b3
WMTM_FIXTURE=${WMTM_FIXTURE:-$SCRATCH/survey/wmtm/tests/fixtures/synthetic/wmtm_state.json}
WMTM_FIXTURE_SHA256=bf85b582248fd8d2480499e54b00ef2ab8fec48f14f367649805ea69b2ba7875
REF=${P_REF:-$ROOT/evidence/s0-2026-11-24/runs/base}
BDIR=${P_BUILD_DIR:-$ROOT/build/p/cetta-$BUILD}
CETTA=$BDIR/cetta
[ -x "$CETTA" ] || { echo "no $CETTA" >&2; exit 1; }
TAG=$BUILD
W1=(deductionrevision flyingraven raveninduction base_rate_acceptance_game_pettachainer direct existential_residual
    machine_failure_inversion raveninduction_implication_pettachainer smokes stationops_fold_inverse toothbrush)

IMG=${P_IMAGE:-$ROOT/build/p/image-$TAG}
if [ -z "${P_KEEP_IMAGE:-}" ] || [ ! -x "$IMG/cetta" ]; then
  rm -rf "$IMG"
  mkdir -p "$IMG"
  git -C "$CHAINER_REPO" archive "$CHAINER_REV" pettachainer/metta | tar -x -C "$IMG" --strip-components=2
  git -C "$PETTA_REPO" archive "$PETTA_REV" lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
  python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w2_wmtm.metta" w2-wmtm \
    --fixture "$WMTM_FIXTURE" --fixture-sha256 "$WMTM_FIXTURE_SHA256" > /dev/null
  python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w3_benchgen.metta" w3-benchgen > /dev/null
fi
cp "$CETTA" "$IMG/cetta"

files_for() {
  case "$1" in
    w1-examples) for n in "${W1[@]}"; do echo "examples/$n.metta"; done ;;
    w2-wmtm) echo examples/s0_w2_wmtm.metta ;;
    w3-benchgen) echo examples/s0_w3_benchgen.metta ;;
    *) echo "unknown workload $1" >&2; exit 2 ;;
  esac
}

{
  echo "tag=$TAG build=$BUILD cetta=$CETTA"
  echo "cetta_sha256=$(sha256sum "$CETTA" | cut -d' ' -f1)"
  echo "cetta_build_head=$(cat "$BDIR/.p-build-head" 2>/dev/null | head -1)"
  echo "cetta_build_patches=$(cat "$BDIR/.p-build-patches" 2>/dev/null | tr '\n' ' ')"
  echo "cetta_build_args=$(cat "$BDIR/.p-build-args" 2>/dev/null | head -1)"
  echo "chainer_rev=$CHAINER_REV petta_rev=$PETTA_REV"
  echo "cettapack_head=$(git -C "$ROOT" rev-parse HEAD) dirty=$(git -C "$ROOT" status --porcelain --untracked-files=no | wc -l)"
  echo "host=$(hostname) nproc=$(nproc) date=$(date -u +%FT%TZ) load=$(cut -d' ' -f1-3 /proc/loadavg)"
  for f in "$IMG"/examples/s0_*.metta; do echo "file $(basename "$f") sha256=$(sha256sum "$f" | cut -d' ' -f1)"; done
} >> "$OUT/env-$TAG.txt"

for w in "${WORKLOADS[@]}"; do
  mkdir -p "$OUT/$w"
  for f in $(files_for "$w"); do
    n=$(basename "$f" .metta)
    for r in $(seq "$RUN_FIRST" $((RUN_FIRST + RUNS - 1))); do
      t=$n.$TAG.r$r
      extra=()
      args=(--lang petta --profile extended --space-engine native)
      case "$BUILD" in
        ev) extra+=(CETTA_EVPROF_OUT="$OUT/$w/$t.evprof.json") ;;
        stats) args=(--emit-runtime-stats "${args[@]}") ;;
        samp) extra+=(CETTA_FNPROF_OUT="$OUT/$w/$t.fnprof.json" CETTA_FNPROF_SAMPLE_US="${SAMPLE_US:-1000}"
                      CETTA_FNPROF_SAMPLES_OUT="$OUT/$w/$t.samples.txt") ;;
      esac
      rm -f "$OUT/$w/$t.evprof.json"
      set +e
      ( cd "$IMG" && env CETTA_PETTA_SEARCH_MACHINE=1 "${extra[@]}" \
          /usr/bin/time -f '%e %U %S %M' -o "$OUT/$w/$t.time" \
          timeout "${TIMEOUT:-1800}" ./cetta "${args[@]}" "$f" \
          > "$OUT/$w/$t.stdout" 2> "$OUT/$w/$t.stderr" )
      echo "$?" > "$OUT/$w/$t.exit"
      set -e
      sha=$(sha256sum "$OUT/$w/$t.stdout" | cut -d' ' -f1)
      ref=$(sha256sum "$REF/$w/$n.r1.stdout" 2>/dev/null | cut -d' ' -f1)
      same=$([ -n "$ref" ] && [ "$sha" = "$ref" ] && echo same || echo DIFFERENT)
      fails=$(grep -c '❌' "$OUT/$w/$t.stdout" || true)
      route=""
      if [ "$BUILD" = ev ]; then
        route=" route=$(python3 -c 'import json,sys
try:
  d=json.load(open(sys.argv[1])); s=sum(x["self"] for x in d["sites"]); n=sum(x["calls"] for x in d["sites"])
  ok = d["format"]=="cetta-evprof-v1" and n>0 and d["overflow"]==0 and d["top_incl_ticks"]<=d["total_ticks"]
  print("ok" if ok else "FAILED", "frames=%d covered=%.3f overflow=%d unwound=%d" % (n, d["top_incl_ticks"]/d["total_ticks"], d["overflow"], d["unwound"]))
except Exception as e: print("MISSING")' "$OUT/$w/$t.evprof.json")"
      elif [ "$BUILD" = stats ]; then
        route=" route=$(grep -q 'runtime' "$OUT/$w/$t.stderr" && echo ok || echo MISSING)"
      fi
      echo "exit=$(cat "$OUT/$w/$t.exit") stdout=$sha answers_vs_s0_base=$same failed_tests=$fails$route" > "$OUT/$w/$t.check"
      echo "$TAG $w $t $(cat "$OUT/$w/$t.check") time=$(tail -1 "$OUT/$w/$t.time")"
    done
  done
done