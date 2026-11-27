| change | group | basis | fraction removed | w1-examples touched / saved | w2-wmtm touched / saved | w3-benchgen touched / saved |
|---|---|---|---|---|---|---|
| C1 | per-call admission, safety and dispatch-mode checks (validation of 5-10 revision fields, table-safety scans, specializer admission, arity lookups through Prolog) | assumed | 0.60 | 17.2% / 10.3% | 16.4% / 9.8% | 14.4% / 8.7% |
| **C1 total** | Call-preparation decisions cached per call site and invalidated per head | | | 17.2% / **10.3% (1.12x)** | 16.4% / **9.8% (1.11x)** | 14.4% / **8.7% (1.09x)** |
| C2 | the prepared-pure call path: refusal checks and cache walks, compile attempts, prepared execution (saving measured end to end with the path off, CeTTa's own reference switch; an adaptive policy does at least as well where its bookkeeping is cheap) | measured | measured | 12.1% / 10.3% (f=0.85) | 8.1% / 8.2% (f=1.01) | 10.5% / 10.7% (f=1.02) |
| **C2 total** | Prepared-call fast path admitted only where it pays (per-head), with an indexed cache (0012) and programs kept across unrelated program changes | | | 12.1% / **10.3% (1.11x)** | 8.1% / **8.2% (1.09x)** | 10.5% / **10.7% (1.12x)** |
| C3 | malloc/calloc/realloc/free and arena growth (saving measured end to end by the mimalloc swap, which also speeds SWI-Prolog's and libc's own allocations) | measured | measured | 8.9% / 8.5% (f=0.95) | 8.9% / 9.7% (f=1.08) | 9.1% / 7.7% (f=0.85) |
| **C3 total** | Allocation: a faster general allocator (mimalloc), then pools for per-call machine records | | | 8.9% / **8.5% (1.09x)** | 8.9% / **9.7% (1.11x)** | 9.1% / **7.7% (1.08x)** |
| C4 | bindings application, deep copies, frame instantiation | assumed | 0.35 | 10.6% / 3.7% | 10.8% / 3.8% | 11.1% / 3.9% |
| **C4 total** | Copying: share ground and settled subterms instead of re-instantiating them | | | 10.6% / **3.7% (1.04x)** | 10.8% / **3.8% (1.04x)** | 11.1% / **3.9% (1.04x)** |
| **all four** | | | | **32.8% (1.49x)** | **31.4% (1.46x)** | **30.9% (1.45x)** |
