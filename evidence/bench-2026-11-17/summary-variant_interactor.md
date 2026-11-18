| mode | processes | queries/batch | answers/batch | per-query us (median) | per-query us (range) | peak RSS MiB (median) | peak RSS MiB (range) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | index build (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 200 | 9835 | 181.39 | 167.34-228.66 | 44.6 | 44.2-44.8 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| hosted-generic | 7 | 200 | 9835 | 102.80 | 93.45-191.84 | 43.3 | 43.0-43.4 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| kernel | 7 | 200 | 9835 | 113.15 | 101.58-130.19 | 817.9 | 817.6-818.1 | 147.4 | 12376.8 | 23361.2 | 14953.5 | 0.0 | 0.000 | 47.065 | 742.1 |
| kernel-cursor | 7 | 200 | 9835 | 137.11 | 131.84-154.82 | 817.7 | 817.6-818.0 | 214.7 | 37251.0 | 20731.0 | 14858.1 | 0.0 | 0.000 | 47.065 | 721.0 |
| fallback | 7 | 200 | 9835 | 225.84 | 219.75-265.02 | 64.3 | 64.2-64.4 | 170.8 | 0.0 | 0.0 | 0.0 | 178282.9 | 1.000 | 0.000 | 240.1 |

- kernel vs generic CeTTa (same pin), end to end: 1.60x per query (>1 means faster than generic); paired by run: median 1.60x, range 1.37-1.85x, faster in 7/7 runs
- kernel-cursor vs generic CeTTa (same pin), end to end: 1.32x per query (>1 means faster than generic); paired by run: median 1.25x, range 1.17-1.73x, faster in 7/7 runs
- hosted-generic vs generic CeTTa (same pin), end to end: 1.76x per query (>1 means faster than generic); paired by run: median 1.63x, range 1.19-1.95x, faster in 7/7 runs
- fallback vs generic CeTTa (same pin), end to end: 0.80x per query (>1 means faster than generic); paired by run: median 0.77x, range 0.68-1.01x, faster in 1/7 runs
- kernel vs CeTTa's matcher behind the same callable (hosted-generic): 0.91x; paired by run: median 0.92x, range 0.75-1.55x, faster in 2/7 runs
- narrowing: bulk 12377 ns/call vs per-byte cursor 37251 ns/call (3.0x less)
