#!/usr/bin/env bash
# Native primitives (t-2026-11-26-cettapack-native-primitives): CeTTa builds for the PeTTaChainer runs, as
# bench/s1/build_s1_cetta.sh (a9b3a811 + upstream/cetta/0001-0004 + cetta-s0/0005, SWI-Prolog interop), in
# separate trees under build/np.
#
#   bench/np/build_np_cetta.sh prof     # 0001-0005 + cetta-s1/0006 (sampler) + cetta-np/*, CETTA_FNPROF=1, -g
#   bench/np/build_np_cetta.sh on       # 0001-0005 + cetta-np/*, CETTA_FNPROF=1 (S0's "on" configuration + ours)
#   bench/np/build_np_cetta.sh off      # 0001-0005 + cetta-np/*, CETTA_FNPROF=0 (profiler compiled out)
#   NP_PATCHES=none bench/np/build_np_cetta.sh ...  # without upstream/cetta-np (S0/S1's binary, for checks)
#
# The native primitives are opt-in: with PeTTaChainer's own code (no shim, bench/np/shim) nothing calls them.
# Environment: GMP_PREFIX, SWIPL_PKG_CONFIG_PATH, JOBS (as build_s0_cetta.sh).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
VARIANT=${1:?prof|on|off}
JOBS=${JOBS:-$(nproc)}
PIN=$(git -C cetta rev-parse HEAD)
SUFFIX=${NP_SUFFIX:-}
BUILD_DIR=$ROOT/build/np/cetta-$VARIANT$SUFFIX
mkdir -p "$ROOT/build/np"
PATCHES=("$ROOT"/upstream/cetta/000[1-4]-*.patch "$ROOT"/upstream/cetta-s0/0005-*.patch)
case "$VARIANT" in
  prof) PATCHES+=("$ROOT"/upstream/cetta-s1/0006-*.patch); FNPROF=(CETTA_FNPROF=1) ;;
  on)   FNPROF=(CETTA_FNPROF=1) ;;
  off)  FNPROF=(CETTA_FNPROF=0) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
if [ "${NP_PATCHES:-all}" != none ]; then
  for p in "$ROOT"/upstream/cetta-np/00*.patch; do [ -e "$p" ] && PATCHES+=("$p"); done
fi
if [ ! -d "$BUILD_DIR/.git" ]; then
  git -c protocol.file.allow=always clone -q --shared "$ROOT/cetta" "$BUILD_DIR"
fi
if [ -z "${NP_NO_RESET:-}" ]; then
  git -C "$BUILD_DIR" checkout -q -f -B cettapack-np-build "$PIN"
  for p in "${PATCHES[@]}"; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q --3way "$p"
  done
fi
if [ "$VARIANT" = prof ]; then
  sed -i 's/^CFLAGS = -O3 -Wall -Werror -std=c11 -pthread$/CFLAGS = -O3 -g -Wall -Werror -std=c11 -pthread/' "$BUILD_DIR/Makefile"
  grep -q '^CFLAGS = -O3 -g ' "$BUILD_DIR/Makefile" || { echo "could not add -g" >&2; exit 1; }
fi
PARENT=$(dirname "$BUILD_DIR")
[ -e "$PARENT/MORK" ] || ln -s "$ROOT/deps/MORK" "$PARENT/MORK"
[ -e "$PARENT/PathMap" ] || ln -s "$ROOT/deps/PathMap" "$PARENT/PathMap"
ARGS=(BUILD=mork CC=gcc ENABLE_LIB_PROLOG=1 "${FNPROF[@]}")
GMP_CFLAGS_V=""
GMP_LDFLAGS_V=""
if [ -n "${GMP_PREFIX:-}" ]; then
  GMP_CFLAGS_V="-I$GMP_PREFIX/include -I$GMP_PREFIX/include/x86_64-linux-gnu"
  GMP_LDFLAGS_V="-L$GMP_PREFIX/lib/x86_64-linux-gnu -L$GMP_PREFIX/lib -lgmp"
fi
GLIBC_MINOR=$(getconf GNU_LIBC_VERSION | sed -E 's/.* 2\.([0-9]+).*/\1/')
if [ "$GLIBC_MINOR" -lt 36 ]; then ARGS+=(PACK_RELATIVE_RELOCS=); fi
GCC_MAJOR=$(gcc -dumpversion | cut -d. -f1)
if [ "$GCC_MAJOR" -lt 12 ]; then
  GMP_CFLAGS_V="$GMP_CFLAGS_V -Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=array-bounds -Wno-error=stringop-overread -Wno-error=restrict"
fi
[ -n "$GMP_CFLAGS_V" ] && ARGS+=(GMP_CFLAGS="$GMP_CFLAGS_V")
[ -n "$GMP_LDFLAGS_V" ] && ARGS+=(GMP_LDFLAGS="$GMP_LDFLAGS_V")
export PKG_CONFIG_PATH=${SWIPL_PKG_CONFIG_PATH:-}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
pkg-config --exists swipl || { echo "swipl.pc not found (set SWIPL_PKG_CONFIG_PATH)" >&2; exit 1; }
make -C "$BUILD_DIR" -j"$JOBS" "${ARGS[@]}"
"$BUILD_DIR/cetta" -v
git -C "$BUILD_DIR" log --format='%H %s' -1 > "$BUILD_DIR/.np-build-head"
printf '%s\n' "${PATCHES[@]##*/}" > "$BUILD_DIR/.np-build-patches"
echo "${ARGS[*]}" > "$BUILD_DIR/.np-build-args"
