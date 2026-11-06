| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | candidates/call | rematches/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 5 | 200 | 1653 | 47.81 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 1199.5 | 42.0 |
| hosted-generic | 5 | 200 | 1653 | 44.20 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 1423.8 | 41.6 |
| kernel | 5 | 200 | 1653 | 56.92 | 127.4 | 9742.6 | 5141.3 | 4098.8 | 0.0 | 9.395 | 10.025 | 488.7 | 1236.2 | 563.1 |
| fallback | 5 | 200 | 1653 | 72.85 | 151.5 | 0.0 | 0.0 | 0.0 | 41663.6 | 0.000 | 0.000 | 147.5 | 1281.6 | 56.8 |

- kernel (guards included) vs generic CeTTa, end to end: 0.84x per query (>1 means the kernel is faster)
- kernel vs CeTTa's generic matcher behind the same callable: 0.78x per query
- in-callable work per call: kernel 23.72 us + guard 0.13 us vs generic matcher 12.04 us
- forced fallback vs generic CeTTa: 1.52x per query (guard + residual)
