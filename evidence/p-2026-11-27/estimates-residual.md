| change | group | basis | fraction removed | w1-examples touched / saved | w2-wmtm touched / saved | w3-benchgen touched / saved |
|---|---|---|---|---|---|---|
| C1 | per-call admission, safety and dispatch-mode checks (validation of 5-10 revision fields, table-safety scans, specializer admission, arity lookups through Prolog) | assumed | 0.60 | 17.1% / 10.3% | 16.1% / 9.6% | 14.3% / 8.6% |
| **C1 total** | Call-preparation decisions cached per call site and invalidated per head | | | 17.1% / **10.3% (1.11x)** | 16.1% / **9.6% (1.11x)** | 14.3% / **8.6% (1.09x)** |
| C2 | the prepared-pure call path: refusal checks and cache walks, compile attempts, prepared execution (saving measured end to end with the path off, CeTTa's own reference switch; an adaptive policy does at least as well where its bookkeeping is cheap) | measured | measured | 12.6% / 10.3% (f=0.82) | 8.8% / 8.2% (f=0.93) | 11.0% / 10.7% (f=0.97) |
| **C2 total** | Prepared-call fast path admitted only where it pays (per-head), with an indexed cache (0012) and programs kept across unrelated program changes | | | 12.6% / **10.3% (1.11x)** | 8.8% / **8.2% (1.09x)** | 11.0% / **10.7% (1.12x)** |
| C3 | malloc/calloc/realloc/free and arena growth (saving measured end to end by the mimalloc swap, which also speeds SWI-Prolog's and libc's own allocations) | measured | measured | 7.6% / 8.5% (f=1.11) | 5.2% / 9.7% (f=1.85) | 7.3% / 7.7% (f=1.05) |
| **C3 total** | Allocation: a faster general allocator (mimalloc), then pools for per-call machine records | | | 7.6% / **8.5% (1.09x)** | 5.2% / **9.7% (1.11x)** | 7.3% / **7.7% (1.08x)** |
| C4 | bindings application, deep copies, frame instantiation | assumed | 0.35 | 11.0% / 3.9% | 11.7% / 4.1% | 11.8% / 4.1% |
| **C4 total** | Copying: share ground and settled subterms instead of re-instantiating them | | | 11.0% / **3.9% (1.04x)** | 11.7% / **4.1% (1.04x)** | 11.8% / **4.1% (1.04x)** |
| **all four** | | | | **32.9% (1.49x)** | **31.6% (1.46x)** | **31.1% (1.45x)** |
