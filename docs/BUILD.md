# Building

```bash
git clone --recurse-submodules <this repo> cettapack && cd cettapack
scripts/build_cetta.sh                 # CeTTa BUILD=mork + bridge, scratch copy, patches applied
make                                   # build/cettapack-host
make test                              # differential tests + certificate skeleton
bench/run_bench.sh                     # benchmark; writes evidence/bench/
```

`scripts/build_cetta.sh` clones the pinned `cetta/` submodule into
`build/cetta-src` (`git clone --shared`), applies `upstream/cetta/*.patch` with
`git am` on a scratch branch there, links `build/MORK` and `build/PathMap` to the
pinned submodules (CeTTa expects them as siblings), and runs
`make BUILD=mork CC=gcc`. It then builds CeTTa's embedding object and writes
`build/cetta.mk` (the object list and flags this repo links against).

This is the MORK/PathMap bridge build: `./build/cetta-src/cetta -v` prints
`cetta 1.5.0-dev (mork)`. No core-only build is used anywhere.

## Host notes (no root needed)

The build host is Ubuntu 22.04 (gcc 11.4, glibc 2.35). Three things needed
handling; none needed root.

1. **GMP headers.** CeTTa includes `<gmp.h>`; the host has `libgmp.so.10` but
   not `libgmp-dev`. Without root:
   ```bash
   mkdir -p ~/gmp && cd ~/gmp && apt-get download libgmp-dev && dpkg -x libgmp-dev_*.deb root
   ln -sf /usr/lib/x86_64-linux-gnu/libgmp.so.10 root/usr/lib/x86_64-linux-gnu/libgmp.so
   GMP_PREFIX=~/gmp/root/usr scripts/build_cetta.sh
   ```
2. **DT_RELR on glibc < 2.36.** CeTTa's Makefile enables
   `-Wl,-z,pack-relative-relocs` when the linker accepts it, but glibc 2.35's
   loader then refuses the bootstrap tools
   (`version 'GLIBC_ABI_DT_RELR' not found`). The script passes
   `PACK_RELATIVE_RELOCS=` when glibc is older than 2.36.
3. **gcc 11 `-Werror` false positives** (`stringop-overflow` in `src/tensor.c`,
   `maybe-uninitialized` in `native/json_cst_value_v1.c`). With gcc < 12 the
   script adds `-Wno-error=` for those warning classes (warnings stay visible).

Items 2 and 3 are CeTTa portability findings; docs/UPSTREAM_NOTES.md has them
written up for an issue.
