| mode | processes | queries/batch | answers/batch | per-query us (median) | per-query us (range) | peak RSS MiB (median) | peak RSS MiB (range) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | index build (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 200 | 1653 | 44.35 | 42.92-47.49 | 42.4 | 42.2-42.5 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| hosted-generic | 7 | 200 | 1653 | 35.22 | 30.95-40.35 | 41.9 | 41.5-41.9 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| kernel | 7 | 200 | 1653 | 39.63 | 38.15-86.19 | 565.4 | 565.1-565.6 | 174.2 | 2457.0 | 3671.1 | 3315.1 | 0.0 | 0.000 | 8.655 | 526.4 |
| kernel-cursor | 7 | 200 | 1653 | 48.21 | 43.07-69.28 | 565.2 | 564.9-565.5 | 209.9 | 7809.1 | 4264.1 | 3467.6 | 0.0 | 0.000 | 8.655 | 497.8 |
| fallback | 7 | 200 | 1653 | 64.00 | 60.15-97.41 | 59.1 | 58.9-59.2 | 182.5 | 0.0 | 0.0 | 0.0 | 37019.8 | 1.000 | 0.000 | 179.3 |
| m1-generic | 7 | 200 | 1653 | 42.75 | 37.88-66.15 | 41.8 | 41.5-41.9 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| m1-hosted-generic | 7 | 200 | 1653 | 37.78 | 34.57-41.33 | 41.3 | 41.1-41.5 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| m1-kernel | 7 | 200 | 1653 | 47.54 | 45.61-54.42 | 563.6 | 563.4-563.9 | 179.3 | 8354.0 | 3678.2 | 3313.9 | 0.0 | 0.000 | 8.655 | 461.6 |
| m1-fallback | 7 | 200 | 1653 | 67.81 | 61.55-71.01 | 57.2 | 57.2-57.6 | 202.6 | 0.0 | 0.0 | 0.0 | 38566.2 | 1.000 | 0.000 | 142.9 |

- kernel vs generic CeTTa (same pin), end to end: 1.12x per query (>1 means faster than generic); paired by run: median 1.11x, range 0.51-1.20x, faster in 4/7 runs
- kernel-cursor vs generic CeTTa (same pin), end to end: 0.92x per query (>1 means faster than generic); paired by run: median 0.91x, range 0.64-1.06x, faster in 2/7 runs
- hosted-generic vs generic CeTTa (same pin), end to end: 1.26x per query (>1 means faster than generic); paired by run: median 1.22x, range 1.08-1.51x, faster in 7/7 runs
- fallback vs generic CeTTa (same pin), end to end: 0.69x per query (>1 means faster than generic); paired by run: median 0.69x, range 0.45-0.75x, faster in 0/7 runs
- kernel vs CeTTa's matcher behind the same callable (hosted-generic): 0.89x; paired by run: median 0.85x, range 0.47-0.91x, faster in 0/7 runs
- kernel (this branch) vs M1-pin generic CeTTa: 1.08x; paired by run: median 1.06x, range 0.44-1.71x, faster in 5/7 runs
- M1 baseline rerun (old pin): kernel vs generic 0.90x per query
- generic CeTTa, new pin vs M1 pin: 0.96x per query (>1 means the new pin is faster)
- kernel, this branch vs M1 kernel: 1.20x per query
- narrowing: bulk 2457 ns/call vs per-byte cursor 7809 ns/call (3.2x less)
