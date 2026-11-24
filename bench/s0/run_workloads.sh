#!/usr/bin/env bash
# S0 workloads (t-2026-11-24-cettapack-s0-profile): real PeTTaChainer runs on CeTTa, synthetic data only.
#
#   bench/s0/run_workloads.sh VARIANT OUTDIR [WORKLOAD ...]
#
# VARIANT: base | off | on (build/s0/cetta-VARIANT, bench/s0/build_s0_cetta.sh). WORKLOADS (default all):
#   w1-examples   PeTTaChainer's own examples (the three in CeTTa's PeTTaChainer compatibility manifest,
#                 tests/petta/chainer_compat/manifest.json at a9b3a811, the other supported examples that
#                 pass on CeTTa, plus W1_EXTRA)
#   w2-wmtm       wmtm-style reasoning run on wmtm's synthetic fixture vocabulary (gen_workloads.py w2-wmtm)
#   w3-benchgen   larger KB: PeTTaChainer's benchgen statements, seeded (gen_workloads.py w3-benchgen)
#
# The PeTTaChainer image is built as CeTTa's compat harness builds it (scripts/petta_chainer_compat.py):
# the pettachainer/metta subtree of PeTTaChainer CHAINER_REV at the root, PeTTa PETTA_REV's lib/lib_*.metta
# overlaid, the cetta binary copied into the image; run with CETTA_PETTA_SEARCH_MACHINE=1,
# --lang petta --profile extended --space-engine native (the harness defaults).
# Per run: OUTDIR/WORKLOAD/NAME.rN.{stdout,stderr,time,exit} and, for VARIANT=on, NAME.rN.fnprof.json.
# RUNS (default 1) repeats each file, numbering the runs from RUN_FIRST (default 1); files run in a fixed order.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
VARIANT=${1:?base|off|on}
OUT=${2:?outdir}
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
CETTA=$ROOT/build/s0/cetta-$VARIANT/cetta
[ -x "$CETTA" ] || { echo "no $CETTA (bench/s0/build_s0_cetta.sh $VARIANT)" >&2; exit 1; }
# the three examples of CeTTa's compat manifest, then every other example b0e24f9 lists in examples/supported.txt
# that gives its stated answers on CeTTa (base_rate_acceptance_game_libpln needs lib_pln and stops at load)
W1=(deductionrevision flyingraven raveninduction base_rate_acceptance_game_pettachainer direct existential_residual
    machine_failure_inversion raveninduction_implication_pettachainer smokes stationops_fold_inverse toothbrush)
read -r -a W1_EXTRA <<< "${W1_EXTRA:-}"

IMG=$ROOT/build/s0/image-$VARIANT
rm -rf "$IMG"
mkdir -p "$IMG"
git -C "$CHAINER_REPO" cat-file -e "$CHAINER_REV^{commit}" 2>/dev/null ||
  git -C "$CHAINER_REPO" fetch -q origin "$CHAINER_REV"
git -C "$CHAINER_REPO" archive "$CHAINER_REV" pettachainer/metta | tar -x -C "$IMG" --strip-components=2
git -C "$PETTA_REPO" archive "$PETTA_REV" lib | tar -x -C "$IMG" --wildcards 'lib/lib_*.metta'
cp "$CETTA" "$IMG/cetta"
# Generated programs sit next to PeTTaChainer's examples: petta_chainer.metta finds its Prolog heap helpers
# from the importing file's directory or its parent (a file one level deeper loads without them and every
# query answers ()).
python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w2_wmtm.metta" w2-wmtm \
  --fixture "$WMTM_FIXTURE" --fixture-sha256 "$WMTM_FIXTURE_SHA256" ${W2_ARGS:-}
python3 "$ROOT/bench/s0/gen_workloads.py" --out "$IMG/examples/s0_w3_benchgen.metta" w3-benchgen ${W3_ARGS:-}

files_for() {
  case "$1" in
    w1-examples) for n in "${W1[@]}" "${W1_EXTRA[@]}"; do [ -n "$n" ] && echo "examples/$n.metta"; done ;;
    w2-wmtm) echo examples/s0_w2_wmtm.metta ;;
    w3-benchgen) echo examples/s0_w3_benchgen.metta ;;
    *) echo "unknown workload $1" >&2; exit 2 ;;
  esac
}

mkdir -p "$OUT"
{
  echo "variant=$VARIANT cetta=$CETTA"
  echo "cetta_sha256=$(sha256sum "$CETTA" | cut -d' ' -f1)"
  echo "cetta_build_head=$(cat "$ROOT/build/s0/cetta-$VARIANT/.s0-build-head" 2>/dev/null)"
  echo "cetta_build_args=$(cat "$ROOT/build/s0/cetta-$VARIANT/.s0-build-args" 2>/dev/null)"
  echo "chainer_rev=$CHAINER_REV petta_rev=$PETTA_REV"
  echo "cettapack_head=$(git -C "$ROOT" rev-parse HEAD)"
  echo "host=$(hostname) nproc=$(nproc) date=$(date -u +%FT%TZ)"
  for f in "$IMG"/examples/s0_*.metta; do echo "generated $(basename "$f") sha256=$(sha256sum "$f" | cut -d' ' -f1)"; done
} > "$OUT/env-$VARIANT.txt"

for w in "${WORKLOADS[@]}"; do
  mkdir -p "$OUT/$w"
  for f in $(files_for "$w"); do
    n=$(basename "$f" .metta)
    for r in $(seq "$RUN_FIRST" $((RUN_FIRST + RUNS - 1))); do
      tag=$n.r$r
      prof=()
      [ "$VARIANT" = on ] && prof=(CETTA_FNPROF_OUT="$OUT/$w/$tag.fnprof.json")
      set +e
      ( cd "$IMG" && env CETTA_PETTA_SEARCH_MACHINE=1 "${prof[@]}" /usr/bin/time -f '%e %U %S %M' -o "$OUT/$w/$tag.time" \
          timeout "${TIMEOUT:-1800}" ./cetta --lang petta --profile extended --space-engine native "$f" \
          > "$OUT/$w/$tag.stdout" 2> "$OUT/$w/$tag.stderr" )
      echo "$?" > "$OUT/$w/$tag.exit"
      set -e
      echo "$VARIANT $w $tag exit=$(cat "$OUT/$w/$tag.exit") time=$(tail -1 "$OUT/$w/$tag.time")"
    done
  done
done
