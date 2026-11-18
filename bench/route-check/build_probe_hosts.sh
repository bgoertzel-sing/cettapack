#!/usr/bin/env bash
# Build route-probe hosts (target_expressed and the option-B sites) for
#   v=a9: this tree's sources (dd3c30a's src, = 4bd2687 code) on CeTTa a9b3a811+0001+0002
#   v=4c: M1's sources (d2df1cc) on CeTTa 4c285a97+M1's 0001
# each against the stats-off objects (identical to the benchmark) and the
# stats-on objects (ENABLE_RUNTIME_STATS=1). Output: build/rc/probe-<v>-stats<0|1>/cettapack-host-<site>
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
V=$1
if [ "$V" = a9 ]; then SRCROOT=$ROOT; SITES="target_expressed gene_variants tissue_fanout variant_interactor"; EXTRA=-DCPK_PROBE_HAS_FORCED_CHECK
else SRCROOT=$ROOT/build/rc/m1src; SITES="target_expressed"; EXTRA=
  rm -rf "$SRCROOT"; mkdir -p "$SRCROOT"; git archive d2df1cc src packs tools upstream Makefile | tar -x -C "$SRCROOT"; fi
# The pack's profile guard compares these with the manifest: take them from the
# benchmarked tree (its initialized cetta submodule and upstream/cetta patches).
if [ "$V" = a9 ]; then PTREE=$ROOT/../m1-numbers; else PTREE=$ROOT/../m1-baseline; fi
PROFILE=$(cd "$PTREE" && python3 tools/cpkc.py --print-profile-components)
P_COMMIT=$(echo "$PROFILE" | sed -n 's/^CPK_CETTA_COMMIT=//p')
P_SHA=$(echo "$PROFILE" | sed -n 's/^CPK_PATCHES_SHA256=//p')
P_MODE=$(echo "$PROFILE" | sed -n 's/^CPK_BUILD_MODE=//p')
for STATS in ${STATS_LIST:-0 1}; do
  MK=$ROOT/build/rc/cetta-$V-stats$STATS.mk
  OUT=$ROOT/build/rc/probe-$V-stats$STATS
  mkdir -p "$OUT/src"
  cp "$SRCROOT"/src/*.c "$SRCROOT"/src/*.h "$OUT/src/"
  cp bench/route-check/route_probe.inc "$OUT/src/"
  python3 bench/route-check/insert_probe.py "$SRCROOT/src/cpk_host.c" "$OUT/src/cpk_host.c"
  # CeTTa's link flags, made absolute against its tree (as the Makefile does)
  C=$(sed -n 's/^CPK_CETTA_ROOT=//p' "$MK")
  CPK_OBJ=$(sed -n 's/^CPK_OBJ=//p' "$MK"); CPK_LDFLAGS=$(sed -n 's/^CPK_LDFLAGS=//p' "$MK")
  CPK_CPPFLAGS=$(sed -n 's/^CPK_CPPFLAGS=//p' "$MK")
  INC=""; for f in $CPK_CPPFLAGS; do case $f in -I/*) INC="$INC $f";; -I*) INC="$INC -I$C/${f#-I}";; -D*) INC="$INC $f";; esac; done
  LASTINC=$(echo $CPK_CPPFLAGS | awk '{print $NF}')
  INC="$INC -include $C/$LASTINC -I$C/include -I$OUT/src"
  OBJS=""; for o in $CPK_OBJ; do OBJS="$OBJS $C/$o"; done
  LD=""; for f in $CPK_LDFLAGS; do case $f in runtime/*) LD="$LD $C/$f";; *) LD="$LD $f";; esac; done
  for s in $SITES; do
    gcc -O2 -Wall -Werror -Wno-format-truncation -std=c11 -pthread $INC $EXTRA \
      -DCPK_CETTA_COMMIT="\"$P_COMMIT\"" -DCPK_PATCHES_SHA256="\"$P_SHA\"" -DCPK_BUILD_MODE="\"$P_MODE\"" -DCPK_CETTA_ROOT="\"$C\"" \
      -DCPK_PACK_DIR="\"$SRCROOT/packs/$s\"" -DCPK_PLAN_SYM=cpk_plan_cpk_site_$s \
      -o "$OUT/cettapack-host-$s" "$OUT"/src/cpk_host.c "$OUT"/src/cpk_pack.c "$OUT"/src/cpk_kernel.c "$OUT"/src/cpk_support.c \
      "$SRCROOT/packs/$s/kernel_plan.c" $OBJS $LD
    echo "built $OUT/cettapack-host-$s"
  done
done
