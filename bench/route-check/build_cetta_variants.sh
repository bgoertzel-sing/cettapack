#!/usr/bin/env bash
# Route check (t-2026-11-17-cettapack-route-check): build CeTTa with runtime
# statistics (ENABLE_RUNTIME_STATS=1) next to the production objects, in copies
# of the two benchmark CeTTa trees, and write a cetta.mk per build for the
# probe hosts. Stats objects get their own names (*.runtime-stats.o), so the
# copied stats-off objects stay exactly as benchmarked.
#   bench/route-check/build_cetta_variants.sh a9|4c
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
V=$1
SRC=$ROOT/build/rc/cetta-$V
GMP_PREFIX=${GMP_PREFIX:-/home/openclaw/research-agent/projects/cettapack/scratch/deps/root/usr}
ARGS=(BUILD=mork CC=gcc PACK_RELATIVE_RELOCS=
  "GMP_CFLAGS=-I$GMP_PREFIX/include -I$GMP_PREFIX/include/x86_64-linux-gnu -Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=array-bounds -Wno-error=stringop-overread -Wno-error=restrict"
  "GMP_LDFLAGS=-L$GMP_PREFIX/lib/x86_64-linux-gnu -L$GMP_PREFIX/lib -lgmp")
for STATS in 0 1; do
  make -C "$SRC" -j${JOBS:-6} "${ARGS[@]}" ENABLE_RUNTIME_STATS=$STATS
  TAG=$(make -s -C "$SRC" -f Makefile -f "$ROOT/scripts/print_vars.mk" print-tag "${ARGS[@]}" ENABLE_RUNTIME_STATS=$STATS)
  ETAG=$TAG$([ $STATS = 1 ] && echo .runtime-stats || true)
  make -C "$SRC" -j${JOBS:-6} "${ARGS[@]}" ENABLE_RUNTIME_STATS=$STATS "runtime/bootstrap/embedding.$ETAG.o"
  OUT=$ROOT/build/rc/cetta-$V-stats$STATS.mk
  make -s -C "$SRC" -f Makefile -f "$ROOT/scripts/print_vars.mk" print-cpk-link "${ARGS[@]}" ENABLE_RUNTIME_STATS=$STATS > "$OUT"
  echo "CPK_CETTA_ROOT=$SRC" >> "$OUT"
  echo "wrote $OUT"
done
