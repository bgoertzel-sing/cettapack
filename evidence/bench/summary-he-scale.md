| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | candidates/call | rematches/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 5 | 40 | 371 | 39.81 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 9.6 | 11.4 |
| kernel | 5 | 40 | 371 | 45.27 | 111.6 | 6910.1 | 2877.4 | 3558.7 | 0.0 | 10.350 | 11.075 | 2.7 | 10.0 | 16.2 |
| he | 5 | 40 | 371 | 1867.75 | - | - | - | - | - | - | - | - | 14.0 | 29.4 |

- kernel (guards included) vs generic CeTTa, end to end: 0.88x per query (>1 means the kernel is faster)
- kernel vs HE: 41.3x per query (HE timed on its first 40 seeds)
