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
4. **Where a query's time goes.** On the 128k-atom benchmark, generic
   `(match ...)` costs about 49 us per query, of which CeTTa's conjunction
   matcher itself (`space_query_conjunction`) is about 11 us; the same matcher
   behind an embedding callable costs about 40 us end to end. Most of the
   per-query cost is evaluation around the matcher (docs/BENCHMARK.md).
5. **Bridge cursor granularity.** Narrowing through
   `cetta_mork_bridge_cursor_*` takes one FFI call per key byte and per value,
   plus an allocation per key read; a "descend prefix and collect values"
   call would make PathMap narrowing from C competitive.
