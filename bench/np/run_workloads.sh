#!/usr/bin/env bash
# Native primitives (t-2026-11-26-cettapack-native-primitives): S0's workloads w1-w3 (bench/s0/run_workloads.sh: the
# same PeTTaChainer b0e24f9b image with the PeTTa e038e4db lib overlay, the same generated programs) on the np builds.
#
#   BUILD=on|off|prof SHIM=none|off|on|check bench/np/run_workloads.sh OUTDIR [WORKLOAD ...]
#
# BUILD: build/np/cetta-$BUILD (bench/np/build_np_cetta.sh; NP_BUILD_DIR overrides the directory).
# SHIM:  none   PeTTaChainer exactly as S0 ran it (one file on the command line)
#        off|on|check  bench/np/shim/np_heaps{_off,,_check}.metta is run first, in the same process
# Per run: OUTDIR/WORKLOAD/NAME.TAG.rN.{stdout,stderr,time,exit,check} (TAG = $BUILD-$SHIM), plus fnprof.json
# (BUILD on/prof), samples.txt (prof), np-heaps.json and np-heaps.log (SHIM on/check; FOREIGN=1 adds
# CETTA_FNPROF_FOREIGN=1).  The check line (sanity gate, per unit): exit status, stdout sha256, whether it
# equals S0's recorded base stdout for the same file (the known answers), failed tests (❌), and for SHIM=check the
# number of heap calls whose native and SWI answers differ (must be 0).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:?on|off|prof}
SHIM=${SHIM:?none|off|on|check}
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
REF=${NP_REF:-$ROOT/evidence/s0-2026-11-24/runs/base}
BDIR=${NP_BUILD_DIR:-$ROOT/build/np/cetta-$BUILD}
CETTA=$BDIR/cetta
[ -x "$CETTA" ] || { echo "no $CETTA" >&2; exit 1; }
TAG=$BUILD-$SHIM
case "$SHIM" in
  none) SHIMFILE= ;;
  on) SHIMFILE=np_heaps.metta ;;
  off) SHIMFILE=np_heaps_off.metta ;;
  check) SHIMFILE=np_heaps_check.metta ;;
  *) echo "unknown shim $SHIM" >&2; exit 2 ;;
esac
W1=(deductionrevision flyingraven raveninduction base_rate_acceptance_game_pettachainer direct existential_residual
    machine_failure_inversion raveninduction_implication_pettachainer smokes stationops_fold_inverse toothbrush)

IMG=${NP_IMAGE:-$ROOT/build/np/image-$TAG}
if [ -z "${NP_KEEP_IMAGE:-}" ] || [ ! -x "$IMG/cetta" ]; then
  rm -rf "$IMG"
  mkdir -p "$IMG"
  git -C "$CHAINER_REPO" archive "$CHAINER_REV" pettachainer/metta | tar -x -C "$IMG" --strip-components=2
  git -C "$PETTA_REPO" archive "$PETTA_REV" lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
  python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w2_wmtm.metta" w2-wmtm \
    --fixture "$WMTM_FIXTURE" --fixture-sha256 "$WMTM_FIXTURE_SHA256" > /dev/null
  python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w3_benchgen.metta" w3-benchgen > /dev/null
  cp "$ROOT"/bench/np/shim/np_heaps*.metta "$IMG/"
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
  echo "tag=$TAG build=$BUILD shim=$SHIM cetta=$CETTA foreign=${FOREIGN:-0}"
  echo "cetta_sha256=$(sha256sum "$CETTA" | cut -d' ' -f1)"
  echo "cetta_build_head=$(cat "$BDIR/.np-build-head" 2>/dev/null | head -1)"
  echo "cetta_build_patches=$(tr '\n' ' ' < "$BDIR/.np-build-patches" 2>/dev/null)"
  echo "cetta_build_args=$(cat "$BDIR/.np-build-args" 2>/dev/null | head -1)"
  echo "chainer_rev=$CHAINER_REV petta_rev=$PETTA_REV"
  echo "cettapack_head=$(git -C "$ROOT" rev-parse HEAD) dirty=$(git -C "$ROOT" status --porcelain --untracked-files=no | wc -l)"
  echo "host=$(hostname) nproc=$(nproc) date=$(date -u +%FT%TZ) load=$(cut -d' ' -f1-3 /proc/loadavg)"
  for f in "$IMG"/examples/s0_*.metta "$IMG"/np_heaps*.metta; do echo "file $(basename "$f") sha256=$(sha256sum "$f" | cut -d' ' -f1)"; done
} >> "$OUT/env-$TAG.txt"

for w in "${WORKLOADS[@]}"; do
  mkdir -p "$OUT/$w"
  for f in $(files_for "$w"); do
    n=$(basename "$f" .metta)
    for r in $(seq "$RUN_FIRST" $((RUN_FIRST + RUNS - 1))); do
      t=$n.$TAG.r$r
      extra=()
      case "$BUILD" in
        on|prof) extra+=(CETTA_FNPROF_OUT="$OUT/$w/$t.fnprof.json") ;;
      esac
      [ "$BUILD" = prof ] && extra+=(CETTA_FNPROF_SAMPLE_US="${SAMPLE_US:-1000}" CETTA_FNPROF_SAMPLES_OUT="$OUT/$w/$t.samples.txt")
      [ "${FOREIGN:-0}" = 1 ] && extra+=(CETTA_FNPROF_FOREIGN=1)
      [ "$SHIM" != none ] && extra+=(CETTA_NP_HEAPS_STATS="$OUT/$w/$t.np-heaps.json" CETTA_NP_HEAPS_LOG="$OUT/$w/$t.np-heaps.log")
      args=(--lang petta --profile extended --space-engine native)
      [ -n "$SHIMFILE" ] && args+=("$SHIMFILE")
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
      heaps=""
      if [ "$SHIM" != none ]; then
        heaps=" heaps=$(python3 -c 'import json,sys
try:
  d=json.load(open(sys.argv[1])); o=d["ops"]
  print("calls:%d,native:%d,declined:%d,checked:%d,mismatched:%d"%tuple(sum(x[k] for x in o) for k in ("calls","native","declined","checked","mismatched")))
except Exception as e: print("MISSING")' "$OUT/$w/$t.np-heaps.json")"
      fi
      # route check (silent-failure guard): with the shim on, heap calls must have been answered natively; in check
      # mode, compared and equal; with it off, no native answer.  MISSING stats = the shim's switch never ran.
      route=""
      case "$SHIM" in
        on) [[ "$heaps" =~ calls:([0-9]+),native:([1-9][0-9]*), ]] && route=" route=ok" || route=" route=FAILED" ;;
        check) [[ "$heaps" =~ checked:([1-9][0-9]*),mismatched:0$ ]] && route=" route=ok" || route=" route=FAILED" ;;
        off) [[ "$heaps" =~ native:0, ]] && route=" route=ok" || route=" route=FAILED" ;;
      esac
      echo "exit=$(cat "$OUT/$w/$t.exit") stdout=$sha answers_vs_s0_base=$same failed_tests=$fails$heaps$route" > "$OUT/$w/$t.check"
      echo "$TAG $w $t $(cat "$OUT/$w/$t.check") time=$(tail -1 "$OUT/$w/$t.time")"
    done
  done
done
