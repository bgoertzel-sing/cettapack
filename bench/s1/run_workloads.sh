#!/usr/bin/env bash
# S1 workloads (t-2026-11-24-cettapack-s1-hot-sites): the S0 workloads w1-w3 (bench/s0/run_workloads.sh: the same
# PeTTaChainer b0e24f9b image with the PeTTa e038e4db lib overlay, the same generated programs) on the S1 builds.
#
#   bench/s1/run_workloads.sh VARIANT OUTDIR [WORKLOAD ...]
#
# VARIANT  binary                      environment
#   base   build/s0/cetta-base/cetta   (a9b3a811 + 0001-0004, as S0's base)
#   off    build/s1/cetta-sites/cetta  CETTA_S1_SITES=0  (compiled sites linked in, switched off: packs off)
#   on     build/s1/cetta-sites/cetta  CETTA_S1_SITES=1  (packs on)
#   prof   build/s1/cetta-prof/cetta   CETTA_FNPROF_OUT, CETTA_FNPROF_SAMPLE_US (default 1000) + _SAMPLES_OUT
#   sprof  build/s1/cetta-sitesprof/cetta  CETTA_S1_SITES=${SPROF_SITES:-1}, CETTA_FNPROF_OUT
# Per run: OUTDIR/WORKLOAD/NAME.rN.{stdout,stderr,time,exit,check} (+ fnprof.json, samples.txt).
# The check line (sanity gate, per unit): exit status, stdout sha256 and whether it equals S0's base stdout for the
# same file (evidence/s0-2026-11-24/runs/base/W/NAME.r1.stdout, the known answers), failed tests (❌).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
VARIANT=${1:?base|off|on|prof|sprof}
OUT=${2:?outdir}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
shift 2
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
REF=${S1_REF:-$ROOT/evidence/s0-2026-11-24/runs/base}
VENV=()
case "$VARIANT" in
  base)  CETTA=$ROOT/build/s0/cetta-base/cetta ;;
  off)   CETTA=$ROOT/build/s1/cetta-sites/cetta; VENV=(CETTA_S1_SITES=0) ;;
  on)    CETTA=$ROOT/build/s1/cetta-sites/cetta; VENV=(CETTA_S1_SITES=1) ;;
  prof)  CETTA=$ROOT/build/s1/cetta-prof/cetta ;;
  sprof) CETTA=$ROOT/build/s1/cetta-sitesprof/cetta; VENV=(CETTA_S1_SITES=${SPROF_SITES:-1}) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
[ -x "$CETTA" ] || { echo "no $CETTA" >&2; exit 1; }
W1=(deductionrevision flyingraven raveninduction base_rate_acceptance_game_pettachainer direct existential_residual
    machine_failure_inversion raveninduction_implication_pettachainer smokes stationops_fold_inverse toothbrush)

IMG=$ROOT/build/s1/image-$VARIANT
rm -rf "$IMG"
mkdir -p "$IMG"
git -C "$CHAINER_REPO" archive "$CHAINER_REV" pettachainer/metta | tar -x -C "$IMG" --strip-components=2
git -C "$PETTA_REPO" archive "$PETTA_REV" lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
cp "$CETTA" "$IMG/cetta"
python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w2_wmtm.metta" w2-wmtm \
  --fixture "$WMTM_FIXTURE" --fixture-sha256 "$WMTM_FIXTURE_SHA256" ${W2_ARGS:-}
python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w3_benchgen.metta" w3-benchgen ${W3_ARGS:-}

files_for() {
  case "$1" in
    w1-examples) for n in "${W1[@]}"; do echo "examples/$n.metta"; done ;;
    w2-wmtm) echo examples/s0_w2_wmtm.metta ;;
    w3-benchgen) echo examples/s0_w3_benchgen.metta ;;
    *) echo "unknown workload $1" >&2; exit 2 ;;
  esac
}

mkdir -p "$OUT"
{
  echo "variant=$VARIANT cetta=$CETTA env=${VENV[*]:-}"
  echo "cetta_sha256=$(sha256sum "$CETTA" | cut -d' ' -f1)"
  echo "cetta_build_head=$(cat "$(dirname "$CETTA")"/.s[01]-build-head 2>/dev/null | head -1)"
  echo "cetta_build_args=$(cat "$(dirname "$CETTA")"/.s[01]-build-args 2>/dev/null | head -1)"
  echo "chainer_rev=$CHAINER_REV petta_rev=$PETTA_REV"
  echo "cettapack_head=$(git -C "$ROOT" rev-parse HEAD) dirty=$(git -C "$ROOT" status --porcelain --untracked-files=no | wc -l)"
  echo "host=$(hostname) nproc=$(nproc) date=$(date -u +%FT%TZ) load=$(cut -d' ' -f1-3 /proc/loadavg)"
  for f in "$IMG"/examples/s0_*.metta; do echo "generated $(basename "$f") sha256=$(sha256sum "$f" | cut -d' ' -f1)"; done
} >> "$OUT/env-$VARIANT.txt"

for w in "${WORKLOADS[@]}"; do
  mkdir -p "$OUT/$w"
  for f in $(files_for "$w"); do
    n=$(basename "$f" .metta)
    for r in $(seq "$RUN_FIRST" $((RUN_FIRST + RUNS - 1))); do
      tag=$n.r$r
      extra=()
      case "$VARIANT" in
        prof) extra=(CETTA_FNPROF_OUT="$OUT/$w/$tag.fnprof.json" CETTA_FNPROF_SAMPLE_US="${SAMPLE_US:-1000}"
                     CETTA_FNPROF_SAMPLES_OUT="$OUT/$w/$tag.samples.txt") ;;
        sprof) extra=(CETTA_FNPROF_OUT="$OUT/$w/$tag.fnprof.json" CETTA_S1_SITES_STATS="$OUT/$w/$tag.sites.json") ;;
        on) extra=(CETTA_S1_SITES_STATS="$OUT/$w/$tag.sites.json") ;;
      esac
      set +e
      ( cd "$IMG" && env CETTA_PETTA_SEARCH_MACHINE=1 "${VENV[@]}" "${extra[@]}" \
          /usr/bin/time -f '%e %U %S %M' -o "$OUT/$w/$tag.time" \
          timeout "${TIMEOUT:-1800}" ./cetta --lang petta --profile extended --space-engine native "$f" \
          > "$OUT/$w/$tag.stdout" 2> "$OUT/$w/$tag.stderr" )
      echo "$?" > "$OUT/$w/$tag.exit"
      set -e
      sha=$(sha256sum "$OUT/$w/$tag.stdout" | cut -d' ' -f1)
      ref=$(sha256sum "$REF/$w/$n.r1.stdout" 2>/dev/null | cut -d' ' -f1)
      same=$([ -n "$ref" ] && [ "$sha" = "$ref" ] && echo same || echo DIFFERENT)
      fails=$(grep -c '❌' "$OUT/$w/$tag.stdout" || true)
      echo "exit=$(cat "$OUT/$w/$tag.exit") stdout=$sha answers_vs_s0_base=$same failed_tests=$fails" > "$OUT/$w/$tag.check"
      echo "$VARIANT $w $tag $(cat "$OUT/$w/$tag.check") time=$(tail -1 "$OUT/$w/$tag.time")"
    done
  done
done
