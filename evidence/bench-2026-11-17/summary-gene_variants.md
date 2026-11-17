| mode | processes | queries/batch | answers/batch | per-query us (median) | per-query us (range) | peak RSS MiB (median) | peak RSS MiB (range) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | index build (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 200 | 7377 | 118.04 | 108.68-136.51 | 42.7 | 42.6-42.9 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| hosted-generic | 7 | 200 | 7377 | 78.28 | 62.74-86.73 | 41.9 | 41.7-42.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| kernel | 7 | 200 | 7377 | 84.98 | 71.41-93.75 | 565.1 | 564.9-565.2 | 179.2 | 7264.2 | 14502.7 | 11970.2 | 0.0 | 0.000 | 38.485 | 496.0 |
| kernel-cursor | 7 | 200 | 7377 | 102.61 | 91.80-187.35 | 565.1 | 564.7-565.2 | 155.9 | 23517.6 | 15274.4 | 11915.4 | 0.0 | 0.000 | 38.485 | 496.6 |
| fallback | 7 | 200 | 7377 | 176.82 | 148.94-182.11 | 58.6 | 58.5-58.8 | 218.5 | 0.0 | 0.0 | 0.0 | 131425.4 | 1.000 | 0.000 | 183.9 |

- kernel vs generic CeTTa (same pin), end to end: 1.39x per query (>1 means faster than generic); paired by run: median 1.46x, range 1.29-1.55x, faster in 7/7 runs
- kernel-cursor vs generic CeTTa (same pin), end to end: 1.15x per query (>1 means faster than generic); paired by run: median 1.06x, range 0.70-1.34x, faster in 5/7 runs
- hosted-generic vs generic CeTTa (same pin), end to end: 1.51x per query (>1 means faster than generic); paired by run: median 1.51x, range 1.37-1.88x, faster in 7/7 runs
- fallback vs generic CeTTa (same pin), end to end: 0.67x per query (>1 means faster than generic); paired by run: median 0.73x, range 0.60-0.83x, faster in 0/7 runs
- kernel vs CeTTa's matcher behind the same callable (hosted-generic): 0.92x; paired by run: median 0.92x, range 0.70-1.07x, faster in 2/7 runs
- narrowing: bulk 7264 ns/call vs per-byte cursor 23518 ns/call (3.2x less)
