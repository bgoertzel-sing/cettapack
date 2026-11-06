# CeTTa feedback found during M1 (for the humans to file as issues)

These are observations on zariuq/CeTTa `4c285a97`; no issue has been filed.

1. **DT_RELR probe accepts what the loader rejects (glibc < 2.36).**
   `Makefile:701` enables `-Wl,-z,pack-relative-relocs` when a link test
   succeeds. On Ubuntu 22.04 (binutils 2.38, glibc 2.35) the link succeeds but
   running the bootstrap tool fails:
   `runtime/langdef-compile-v1-mork...: /lib/x86_64-linux-gnu/libc.so.6: version 'GLIBC_ABI_DT_RELR' not found`.
   Suggestion: run the probe binary (or check `GLIBC_ABI_DT_RELR` in libc)
   rather than only linking it. Workaround: `make PACK_RELATIVE_RELOCS=`.
2. **gcc 11 `-Werror` false positives.** With gcc 11.4:
   `src/tensor.c:212` `-Werror=stringop-overflow` (inlined
   `native_storage_value` writing into an 8-byte `CettaTensorScalar`), and
   `native/json_cst_value_v1.c:295,518` `-Werror=maybe-uninitialized` (`scalar`).
   gcc 12+ may not raise them; either initialize or document a minimum gcc.
3. **Missing `gmp.h` is reported as a compile error late in the build.**
   `make doctor-bridge` could check for the GMP headers up front.
4. **Callable dispatch cost.** In this repo's benchmark, a registered
   embedding callback that does nothing but return precomputed answers costs
   about as much per call as the whole generic `match` on a 128k-atom space
   (docs/BENCHMARK.md). A cheaper in-process path for compiled kernels would
   matter more than the kernel itself for small answer sets.
