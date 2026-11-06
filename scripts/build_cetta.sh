#!/usr/bin/env bash
# Build CeTTa (pinned submodule + upstream/cetta patches) with its MORK/PathMap
# bridge, in a scratch copy. The submodule's history is never touched.
#
#   scripts/build_cetta.sh            # -> build/cetta-src, build/cetta.mk
#
# Environment:
#   CETTA_BUILD_DIR   scratch copy (default build/cetta-src)
#   GMP_PREFIX        prefix holding include/gmp.h and lib/libgmp.so when the
#                     system has no libgmp-dev (see docs/BUILD.md)
#   JOBS              parallel jobs (default: nproc)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
BUILD_DIR=${CETTA_BUILD_DIR:-$ROOT/build/cetta-src}
JOBS=${JOBS:-$(nproc)}
PIN=$(git -C cetta rev-parse HEAD)

mkdir -p "$ROOT/build"
if [ ! -d "$BUILD_DIR/.git" ]; then
  git clone -q --shared "$ROOT/cetta" "$BUILD_DIR"
fi
# apply the patch series on a scratch branch of the scratch copy
if [ "$(git -C "$BUILD_DIR" log -1 --format=%s 2>/dev/null)" != "$(sed -n 's/^Subject: \[PATCH\] //p' "$(ls "$ROOT"/upstream/cetta/*.patch | tail -1)")" ]; then
  git -C "$BUILD_DIR" checkout -q -B cettapack-build "$PIN"
  for p in "$ROOT"/upstream/cetta/*.patch; do
    git -C "$BUILD_DIR" -c user.name=cettapack -c user.email=cettapack@localhost am -q "$p"
  done
fi
# CeTTa's bridge expects MORK and PathMap next to the CeTTa tree
PARENT=$(dirname "$BUILD_DIR")
[ -e "$PARENT/MORK" ] || ln -s "$ROOT/deps/MORK" "$PARENT/MORK"
[ -e "$PARENT/PathMap" ] || ln -s "$ROOT/deps/PathMap" "$PARENT/PathMap"

ARGS=(BUILD=mork CC=gcc)
GMP_CFLAGS_V=""
GMP_LDFLAGS_V=""
if [ -n "${GMP_PREFIX:-}" ]; then
  GMP_CFLAGS_V="-I$GMP_PREFIX/include -I$GMP_PREFIX/include/x86_64-linux-gnu"
  GMP_LDFLAGS_V="-L$GMP_PREFIX/lib/x86_64-linux-gnu -L$GMP_PREFIX/lib -lgmp"
fi
# Host portability (docs/BUILD.md): glibc < 2.36 cannot load the DT_RELR
# binaries the linker produces; gcc 11 raises -Werror false positives.
GLIBC_MINOR=$(getconf GNU_LIBC_VERSION | sed -E 's/.* 2\.([0-9]+).*/\1/')
if [ "$GLIBC_MINOR" -lt 36 ]; then ARGS+=(PACK_RELATIVE_RELOCS=); fi
GCC_MAJOR=$(gcc -dumpversion | cut -d. -f1)
if [ "$GCC_MAJOR" -lt 12 ]; then
  GMP_CFLAGS_V="$GMP_CFLAGS_V -Wno-error=stringop-overflow -Wno-error=maybe-uninitialized -Wno-error=array-bounds -Wno-error=stringop-overread -Wno-error=restrict"
fi
[ -n "$GMP_CFLAGS_V" ] && ARGS+=(GMP_CFLAGS="$GMP_CFLAGS_V")
[ -n "$GMP_LDFLAGS_V" ] && ARGS+=(GMP_LDFLAGS="$GMP_LDFLAGS_V")

make -C "$BUILD_DIR" -j"$JOBS" "${ARGS[@]}"
TAG=$(make -s -C "$BUILD_DIR" -f Makefile -f "$ROOT/scripts/print_vars.mk" print-tag "${ARGS[@]}")
make -C "$BUILD_DIR" -j"$JOBS" "${ARGS[@]}" "runtime/bootstrap/embedding.$TAG.o"
make -s -C "$BUILD_DIR" -f Makefile -f "$ROOT/scripts/print_vars.mk" print-cpk-link "${ARGS[@]}" > "$ROOT/build/cetta.mk"
echo "CPK_CETTA_ROOT=$BUILD_DIR" >> "$ROOT/build/cetta.mk"
"$BUILD_DIR/cetta" -v
echo "build/cetta.mk written"
