#!/usr/bin/env bash
# S2 (t-2026-11-24-cettapack-s2-agenda-functions): CeTTa builds with the compiled PeTTaChainer functions, as
# bench/s1/build_s1_cetta.sh (a9b3a811 + upstream/cetta/0001-0004 + cetta-s0/0005 + cetta-s1/0006-0007) plus
# upstream/cetta-s2/0008 (the function-entry hook), in separate trees under build/s2.
#
#   bench/s2/build_s2_cetta.sh fns      # CETTA_FNPROF=0 CETTA_S1_SITES=1 CETTA_S2_FNS=1, with cettapack's compiled
#                                       #   S1 site (src/cpk_s1_sites.c) and S2 functions (src/cpk_s2_fns.c) linked in;
#                                       #   packs off/on is a run-time switch of this one binary (CETTA_S2_FNS=0/1)
#   bench/s2/build_s2_cetta.sh fnsprof  # the same with CETTA_FNPROF=1 and -g (profiles and samples)
#
# Environment: GMP_PREFIX, SWIPL_PKG_CONFIG_PATH, JOBS (as build_s1_cetta.sh).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
VARIANT=${1:?fns|fnsprof}
JOBS=${JOBS:-$(nproc)}
PIN=$(git -C cetta rev-parse HEAD)
BUILD_DIR=$ROOT/build/s2/cetta-$VARIANT
mkdir -p "$ROOT/build/s2"
PATCHES=("$ROOT"/upstream/cetta/000[1-4]-*.patch "$ROOT"/upstream/cetta-s0/0005-*.patch
         "$ROOT"/upstream/cetta-s1/0006-*.patch "$ROOT"/upstream/cetta-s1/0007-*.patch "$ROOT"/upstream/cetta-s2/0008-*.patch)
case "$VARIANT" in
  fns)     FNPROF=(CETTA_FNPROF=0 CETTA_S1_SITES=1 CETTA_S2_FNS=1) ;;
  fnsprof) FNPROF=(CETTA_FNPROF=1 CETTA_S1_SITES=1 CETTA_S2_FNS=1) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
if [ ! -d "$BUILD_DIR/.git" ]; then
  git -c protocol.file.allow=always clone -q --shared "$ROOT/cetta" "$BUILD_DIR"
fi
if [ -z "${S2_NO_RESET:-}" ]; then
  git -C "$BUILD_DIR" checkout -q -f -B cettapack-s2-build "$PIN"
  for p in "${PATCHES[@]}"; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q "$p"
  done
fi
if [ "$VARIANT" = fnsprof ]; then
  sed -i 's/^CFLAGS = -O3 -Wall -Werror -std=c11 -pthread$/CFLAGS = -O3 -g -Wall -Werror -std=c11 -pthread/' "$BUILD_DIR/Makefile"
  grep -q '^CFLAGS = -O3 -g ' "$BUILD_DIR/Makefile" || { echo "could not add -g" >&2; exit 1; }
fi
# cettapack's compiled code goes in as sources of the CeTTa build (0007/0008 name the files and build them in)
cp "$ROOT/src/cpk_s1_sites.c" "$BUILD_DIR/src/cetta_s1_sites.c"
cp "$ROOT/src/cpk_s1_sites_text.h" "$BUILD_DIR/src/cpk_s1_sites_text.h"
cp "$ROOT/src/cpk_s2_fns.c" "$BUILD_DIR/src/cetta_s2_fns.c"
cp "$ROOT/src/cpk_s2_fns_text.h" "$BUILD_DIR/src/cpk_s2_fns_text.h"
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
git -C "$BUILD_DIR" log --format='%H %s' -1 > "$BUILD_DIR/.s2-build-head"
{ echo "${ARGS[*]}"; sha256sum "$ROOT/src/cpk_s2_fns.c" "$ROOT/src/cpk_s2_fns_text.h" "$ROOT/src/cpk_s1_sites.c" | sed "s|$ROOT/||"; } > "$BUILD_DIR/.s2-build-args"
