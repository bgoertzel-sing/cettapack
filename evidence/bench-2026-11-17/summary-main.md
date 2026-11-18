| mode | processes | queries/batch | answers/batch | per-query us (median) | per-query us (range) | peak RSS MiB (median) | peak RSS MiB (range) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | index build (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 200 | 1653 | 46.20 | 41.66-48.09 | 42.4 | 42.3-42.5 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| hosted-generic | 7 | 200 | 1653 | 34.60 | 31.86-47.09 | 41.8 | 41.6-42.1 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| kernel | 7 | 200 | 1653 | 37.27 | 33.94-43.99 | 565.5 | 565.0-565.6 | 141.9 | 2333.6 | 3445.3 | 3098.7 | 0.0 | 0.000 | 8.655 | 503.6 |
| kernel-cursor | 7 | 200 | 1653 | 46.73 | 45.35-50.82 | 565.5 | 565.1-565.5 | 209.2 | 8315.5 | 4013.4 | 3644.1 | 0.0 | 0.000 | 8.655 | 510.7 |
| fallback | 7 | 200 | 1653 | 64.03 | 59.04-71.45 | 59.1 | 58.9-59.3 | 177.7 | 0.0 | 0.0 | 0.0 | 37526.0 | 1.000 | 0.000 | 177.2 |
| m1-generic | 7 | 200 | 1653 | 39.80 | 38.97-45.35 | 41.6 | 41.6-41.8 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| m1-hosted-generic | 7 | 200 | 1653 | 38.20 | 35.47-41.05 | 41.0 | 40.8-41.1 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| m1-kernel | 7 | 200 | 1653 | 46.95 | 42.77-52.26 | 563.1 | 562.7-563.1 | 169.1 | 8327.7 | 3501.4 | 3191.2 | 0.0 | 0.000 | 8.655 | 476.7 |
| m1-fallback | 7 | 200 | 1653 | 70.34 | 63.41-73.95 | 56.8 | 56.7-57.0 | 211.4 | 0.0 | 0.0 | 0.0 | 38809.5 | 1.000 | 0.000 | 147.5 |

- kernel vs generic CeTTa (same pin), end to end: 1.24x per query (>1 means faster than generic); paired by run: median 1.16x, range 1.07-1.28x, faster in 7/7 runs
- kernel-cursor vs generic CeTTa (same pin), end to end: 0.99x per query (>1 means faster than generic); paired by run: median 0.99x, range 0.82-1.04x, faster in 3/7 runs
- hosted-generic vs generic CeTTa (same pin), end to end: 1.34x per query (>1 means faster than generic); paired by run: median 1.30x, range 0.98-1.50x, faster in 6/7 runs
- fallback vs generic CeTTa (same pin), end to end: 0.72x per query (>1 means faster than generic); paired by run: median 0.71x, range 0.60-0.76x, faster in 0/7 runs
- kernel vs CeTTa's matcher behind the same callable (hosted-generic): 0.93x; paired by run: median 0.99x, range 0.73-1.26x, faster in 3/7 runs
- kernel (this branch) vs M1-pin generic CeTTa: 1.07x; paired by run: median 1.05x, range 0.89-1.34x, faster in 4/7 runs
- M1 baseline rerun (old pin): kernel vs generic 0.85x per query
- generic CeTTa, new pin vs M1 pin: 0.86x per query (>1 means the new pin is faster)
- kernel, this branch vs M1 kernel: 1.26x per query
- narrowing: bulk 2334 ns/call vs per-byte cursor 8316 ns/call (3.6x less)
