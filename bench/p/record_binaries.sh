#!/usr/bin/env bash
# cetta-internals P: record the binaries of the builds (sha256, build head, patches, make args) and the patch files.
#   bench/p/record_binaries.sh > evidence/p-2026-11-27/binaries.txt
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
for v in off ev stats samp off-proto ev-proto; do
  d=build/p/cetta-$v
  [ -x "$d/cetta" ] || continue
  echo "$(sha256sum "$d/cetta" | cut -d' ' -f1)  build/p/cetta-$v/cetta"
  echo "build/p/cetta-$v head: $(cat "$d/.p-build-head" 2>/dev/null)"
  echo "build/p/cetta-$v patches: $(tr '\n' ' ' < "$d/.p-build-patches" 2>/dev/null)"
  echo "build/p/cetta-$v args: $(cat "$d/.p-build-args" 2>/dev/null)"
done
sha256sum upstream/cetta/000[1-4]-*.patch upstream/cetta-s0/0005-*.patch upstream/cetta-s1/0006-*.patch \
  upstream/cetta-p/0011-*.patch upstream/cetta-p-proto/0012-*.patch
M=../../scratch/deps/mimalloc-2.1.7
[ -e "$M/out/libmimalloc.so.2.1" ] && echo "$(sha256sum "$M/out/libmimalloc.so.2.1" | cut -d' ' -f1)  mimalloc 2.1.7 libmimalloc.so.2.1 (git $(git -C "$M" rev-parse HEAD), cmake Release)"
echo "CeTTa pin: a9b3a81186529fd99649e437809bdb6f58fce2fd"
echo "SWI-Prolog: $(PKG_CONFIG_PATH=${SWIPL_PKG_CONFIG_PATH:-} pkg-config --modversion swipl 2>/dev/null || echo 10.0.2)"
echo "Note: every build is -O3 -g (bench/p/build_p_cetta.sh); stats is CeTTa's runtime/cetta-mork-runtime-stats copied to cetta."
