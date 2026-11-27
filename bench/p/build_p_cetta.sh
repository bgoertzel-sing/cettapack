#!/usr/bin/env bash
# cetta-internals P (t-2026-11-27-cetta-internals-p-profile): CeTTa builds for the evaluator profile, as
# bench/np/build_np_cetta.sh (a9b3a811 + upstream/cetta/0001-0004, SWI-Prolog interop, -g), in separate trees under
# build/p.
#
#   bench/p/build_p_cetta.sh ev      # 0001-0004 + cetta-p/0011, CETTA_EVPROF=1 (phase timers on)
#   bench/p/build_p_cetta.sh off     # 0001-0004 + cetta-p/0011, CETTA_EVPROF=0 (timers compiled out: the reference)
#   bench/p/build_p_cetta.sh stats   # 0001-0004 + cetta-p/0011, CETTA_EVPROF=0, ENABLE_RUNTIME_STATS=1 (CeTTa's counters)
#   bench/p/build_p_cetta.sh samp    # 0001-0004 + cetta-s0/0005 + cetta-s1/0006, CETTA_FNPROF=1 (S1's sampler)
#   P_EXTRA_PATCHES="a.patch b.patch" P_SUFFIX=-x bench/p/build_p_cetta.sh off   # + prototype patches (Do 4)
#
# Every variant is built with -O3 -g (-g does not change the machine code; it gives the sampler inlined frames).
# Environment: GMP_PREFIX, SWIPL_PKG_CONFIG_PATH, JOBS, CETTA_SRC (a CeTTa checkout holding a9b3a811; default
# $ROOT/cetta).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
VARIANT=${1:?ev|off|stats|samp}
JOBS=${JOBS:-$(nproc)}
PIN=a9b3a81186529fd99649e437809bdb6f58fce2fd
CETTA_SRC=${CETTA_SRC:-$ROOT/cetta}
SUFFIX=${P_SUFFIX:-}
BUILD_DIR=$ROOT/build/p/cetta-$VARIANT$SUFFIX
mkdir -p "$ROOT/build/p"
PATCHES=("$ROOT"/upstream/cetta/000[1-4]-*.patch)
ARGS=(BUILD=mork CC=gcc ENABLE_LIB_PROLOG=1)
case "$VARIANT" in
  ev)    PATCHES+=("$ROOT"/upstream/cetta-p/0011-*.patch); ARGS+=(CETTA_EVPROF=1) ;;
  off)   PATCHES+=("$ROOT"/upstream/cetta-p/0011-*.patch); ARGS+=(CETTA_EVPROF=0) ;;
  stats) PATCHES+=("$ROOT"/upstream/cetta-p/0011-*.patch); ARGS+=(CETTA_EVPROF=0 ENABLE_RUNTIME_STATS=1) ;;
  samp)  PATCHES+=("$ROOT"/upstream/cetta-s0/0005-*.patch "$ROOT"/upstream/cetta-s1/0006-*.patch); ARGS+=(CETTA_FNPROF=1) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
read -r -a EXTRA <<< "${P_EXTRA_PATCHES:-}"
for p in "${EXTRA[@]}"; do [ -n "$p" ] && PATCHES+=("$p"); done
if [ ! -d "$BUILD_DIR/.git" ]; then
  git -c protocol.file.allow=always clone -q --shared "$CETTA_SRC" "$BUILD_DIR"
fi
if [ -z "${P_NO_RESET:-}" ]; then
  git -C "$BUILD_DIR" checkout -q -f -B cettapack-p-build "$PIN"
  for p in "${PATCHES[@]}"; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q --3way "$p"
  done
fi
sed -i 's/^CFLAGS = -O3 -Wall -Werror -std=c11 -pthread$/CFLAGS = -O3 -g -Wall -Werror -std=c11 -pthread/' "$BUILD_DIR/Makefile"
grep -q '^CFLAGS = -O3 -g ' "$BUILD_DIR/Makefile" || { echo "could not add -g" >&2; exit 1; }
PARENT=$(dirname "$BUILD_DIR")
[ -e "$PARENT/MORK" ] || ln -s "$ROOT/deps/MORK" "$PARENT/MORK"
[ -e "$PARENT/PathMap" ] || ln -s "$ROOT/deps/PathMap" "$PARENT/PathMap"
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
git -C "$BUILD_DIR" log --format='%H %s' -1 > "$BUILD_DIR/.p-build-head"
printf '%s\n' "${PATCHES[@]##*/}" > "$BUILD_DIR/.p-build-patches"
echo "${ARGS[*]}" > "$BUILD_DIR/.p-build-args"
