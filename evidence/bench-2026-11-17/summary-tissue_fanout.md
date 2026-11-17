| mode | processes | queries/batch | answers/batch | per-query us (median) | per-query us (range) | peak RSS MiB (median) | peak RSS MiB (range) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | index build (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 40 | 173693 | 16269.67 | 14849.26-17086.56 | 51.7 | 51.7-51.9 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| hosted-generic | 7 | 40 | 173693 | 7779.84 | 7153.64-8184.34 | 46.6 | 46.4-46.7 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 |
| kernel | 7 | 40 | 173693 | 9291.71 | 8361.54-9454.65 | 570.9 | 570.5-571.1 | 1086.1 | 927905.7 | 2857387.1 | 1964744.6 | 0.0 | 0.000 | 4255.350 | 500.1 |
| kernel-cursor | 7 | 40 | 173693 | 10822.04 | 10398.66-11356.02 | 570.8 | 570.7-571.0 | 1126.2 | 2421441.0 | 2925228.3 | 1979461.2 | 0.0 | 0.000 | 4255.350 | 546.5 |
| fallback | 7 | 40 | 173693 | 19817.48 | 18974.04-22980.96 | 58.5 | 58.3-58.6 | 1253.3 | 0.0 | 0.0 | 0.0 | 17381522.9 | 1.000 | 0.000 | 180.3 |

- kernel vs generic CeTTa (same pin), end to end: 1.75x per query (>1 means faster than generic); paired by run: median 1.72x, range 1.58-2.04x, faster in 7/7 runs
- kernel-cursor vs generic CeTTa (same pin), end to end: 1.50x per query (>1 means faster than generic); paired by run: median 1.47x, range 1.37-1.57x, faster in 7/7 runs
- hosted-generic vs generic CeTTa (same pin), end to end: 2.09x per query (>1 means faster than generic); paired by run: median 2.08x, range 1.91-2.27x, faster in 7/7 runs
- fallback vs generic CeTTa (same pin), end to end: 0.82x per query (>1 means faster than generic); paired by run: median 0.81x, range 0.72-0.90x, faster in 0/7 runs
- kernel vs CeTTa's matcher behind the same callable (hosted-generic): 0.84x; paired by run: median 0.83x, range 0.80-0.95x, faster in 0/7 runs
- narrowing: bulk 927906 ns/call vs per-byte cursor 2421441 ns/call (2.6x less)
