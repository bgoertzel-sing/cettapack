#!/usr/bin/env bash
# S1 (t-2026-11-24-cettapack-s1-hot-sites): CeTTa builds for the PeTTaChainer runs, as bench/s0/build_s0_cetta.sh
# (a9b3a811 + upstream/cetta/0001-0004, SWI-Prolog interop) in separate trees under build/s1.
#
#   bench/s1/build_s1_cetta.sh prof    # + cetta-s0/0005 + cetta-s1/0006, CETTA_FNPROF=1, -g (sampler; profiles only)
#   bench/s1/build_s1_cetta.sh sites   # + cetta-s0/0005 + cetta-s1/0006-0007 (compiled-site hook, CETTA_FNPROF=0)
#                                      #   with cettapack's compiled S1 sites linked in (src/cpk_s1_sites.c);
#                                      #   packs off/on is a run-time switch of this one binary (CETTA_S1_SITES=0/1)
#   bench/s1/build_s1_cetta.sh sitesprof  # the same with CETTA_FNPROF=1 (the site rows and their guard counts)
#
# Environment: GMP_PREFIX, SWIPL_PKG_CONFIG_PATH, JOBS (as build_s0_cetta.sh).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
VARIANT=${1:?prof|sites|sitesprof}
JOBS=${JOBS:-$(nproc)}
PIN=$(git -C cetta rev-parse HEAD)
BUILD_DIR=$ROOT/build/s1/cetta-$VARIANT
mkdir -p "$ROOT/build/s1"
PATCHES=("$ROOT"/upstream/cetta/000[1-4]-*.patch "$ROOT"/upstream/cetta-s0/0005-*.patch "$ROOT"/upstream/cetta-s1/0006-*.patch)
case "$VARIANT" in
  prof)      FNPROF=(CETTA_FNPROF=1) ;;
  sites)     PATCHES+=("$ROOT"/upstream/cetta-s1/0007-*.patch); FNPROF=(CETTA_FNPROF=0 CETTA_S1_SITES=1) ;;
  sitesprof) PATCHES+=("$ROOT"/upstream/cetta-s1/0007-*.patch); FNPROF=(CETTA_FNPROF=1 CETTA_S1_SITES=1) ;;
  *) echo "unknown variant $VARIANT" >&2; exit 2 ;;
esac
if [ ! -d "$BUILD_DIR/.git" ]; then
  git -c protocol.file.allow=always clone -q --shared "$ROOT/cetta" "$BUILD_DIR"
fi
if [ -z "${S1_NO_RESET:-}" ]; then
  git -C "$BUILD_DIR" checkout -q -f -B cettapack-s1-build "$PIN"
  for p in "${PATCHES[@]}"; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q "$p"
  done
fi
case "$VARIANT" in
  prof)
    # -g for symbolised samples (gcc emits the same code with and without -g); set in the tree's Makefile, since a
    # CFLAGS on make's command line is exported into cargo's C builds (jemalloc fails on -Werror)
    sed -i 's/^CFLAGS = -O3 -Wall -Werror -std=c11 -pthread$/CFLAGS = -O3 -g -Wall -Werror -std=c11 -pthread/' "$BUILD_DIR/Makefile"
    grep -q '^CFLAGS = -O3 -g ' "$BUILD_DIR/Makefile" || { echo "could not add -g" >&2; exit 1; } ;;
  sites|sitesprof)
    # cettapack's compiled sites go in as one more source of the CeTTa build (0007 names the file
    # src/cetta_s1_sites.c and builds it into the binary; the generic hook has no sites without it)
    cp "$ROOT/src/cpk_s1_sites.c" "$BUILD_DIR/src/cetta_s1_sites.c"
    cp "$ROOT/src/cpk_s1_sites_text.h" "$BUILD_DIR/src/cpk_s1_sites_text.h" ;;
esac
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
git -C "$BUILD_DIR" log --format='%H %s' -1 > "$BUILD_DIR/.s1-build-head"
echo "${ARGS[*]}" > "$BUILD_DIR/.s1-build-args"
