| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | answer ns/call | fallback ns/call | candidates/call | rematches/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 5 | 40 | 371 | 46.05 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.0 | 11.0 | 11.2 |
| kernel | 5 | 40 | 371 | 49.24 | 119.7 | 7353.4 | 3050.3 | 3835.8 | 0.0 | 10.350 | 11.075 | 2.8 | 10.3 | 16.3 |
| he | 5 | 40 | 371 | 2589.31 | - | - | - | - | - | - | - | - | 19.3 | 29.5 |

- kernel (guards included) vs generic CeTTa, end to end: 0.94x per query (>1 means the kernel is faster)
- kernel vs HE: 52.6x per query (HE timed on its first 40 seeds)
