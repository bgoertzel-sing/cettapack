#!/usr/bin/env bash
# S0 (t-2026-11-24-cettapack-s0-profile): CeTTa builds for the PeTTaChainer profiles.
# Same pin and patch series as scripts/build_cetta.sh (a9b3a811 + upstream/cetta/0001-0004),
# plus SWI-Prolog interop (ENABLE_LIB_PROLOG=1: PeTTaChainer's callPredicate/assertz/erase and
# heap helpers are Prolog primitives), in separate trees so build/cetta-src (make test) is untouched.
#
#   bench/s0/build_s0_cetta.sh base          # build/s0/cetta-base: 0001-0004, no profiler
#   bench/s0/build_s0_cetta.sh off           # build/s0/cetta-off:  + upstream/cetta-s0/0005, CETTA_FNPROF=0 (default)
#   bench/s0/build_s0_cetta.sh on            # build/s0/cetta-on:   + upstream/cetta-s0/0005, CETTA_FNPROF=1
#
# Environment: GMP_PREFIX (as scripts/build_cetta.sh), SWIPL_PKG_CONFIG_PATH (a directory with a
# swipl.pc; docs/S0_PROFILE.md says how the user-local SWI-Prolog 8.4.2 was unpacked), JOBS.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
VARIANT=${1:?base|off|on}
JOBS=${JOBS:-$(nproc)}
PIN=$(git -C cetta rev-parse HEAD)
BUILD_DIR=$ROOT/build/s0/cetta-$VARIANT
mkdir -p "$ROOT/build/s0"
PATCHES=("$ROOT"/upstream/cetta/000[1-4]-*.patch)
case "$VARIANT" in
  base) FNPROF=() ;;
  off)  PATCHES+=("$ROOT"/upstream/cetta-s0/0005-*.patch); FNPROF=(CETTA_FNPROF=0) ;;
  on)   PATCHES+=("$ROOT"/upstream/cetta-s0/0005-*.patch); FNPROF=(CETTA_FNPROF=1) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
if [ ! -d "$BUILD_DIR/.git" ]; then
  git -c protocol.file.allow=always clone -q --shared "$ROOT/cetta" "$BUILD_DIR"
fi
# S0_NO_RESET=1: rebuild the tree as it is (after replacing a patch by hand), without re-checking-out
if [ -z "${S0_NO_RESET:-}" ]; then
  git -C "$BUILD_DIR" checkout -q -f -B cettapack-s0-build "$PIN"
  for p in "${PATCHES[@]}"; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q "$p"
  done
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
git -C "$BUILD_DIR" log --format='%H %s' -1 > "$BUILD_DIR/.s0-build-head"
echo "${ARGS[*]}" > "$BUILD_DIR/.s0-build-args"
