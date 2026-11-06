| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | candidates/call | rematches/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 5 | 200 | 1653 | 49.04 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 1287.4 | 42.3 |
| hosted-generic | 5 | 200 | 1653 | 40.28 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 1255.1 | 41.4 |
| kernel | 5 | 200 | 1653 | 58.89 | 150.3 | 11605.4 | 6001.3 | 4847.4 | 0.0 | 9.395 | 10.025 | 566.7 | 1588.4 | 563.2 |
| fallback | 5 | 200 | 1653 | 77.10 | 156.4 | 0.0 | 0.0 | 0.0 | 43083.3 | 0.000 | 0.000 | 162.8 | 1313.1 | 56.8 |

- kernel (guards included) vs generic CeTTa, end to end: 0.83x per query (>1 means the kernel is faster)
- kernel vs CeTTa's generic matcher behind the same callable: 0.68x per query
- in-callable work per call: kernel 28.03 us + guard 0.15 us vs generic matcher 10.71 us
- forced fallback vs generic CeTTa: 1.57x per query (guard + residual)
