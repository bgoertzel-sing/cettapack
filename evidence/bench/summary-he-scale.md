| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | multiplicity ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | rematches/call | records/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 40 | 371 | 40.76 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.000 | 0.000 | 0.0 | 9.9 | 11.5 |
| kernel | 7 | 40 | 371 | 45.73 | 120.4 | 6757.0 | 2613.8 | 147.4 | 3375.1 | 0.0 | 0.000 | 9.700 | 9.700 | 10.350 | 2.7 | 9.8 | 16.4 |
| he | 7 | 40 | 371 | 1698.70 | - | - | - | - | - | - | - | - | - | - | - | 13.8 | 29.4 |

- kernel (guards included) vs generic CeTTa, end to end: 0.89x per query (>1 means the kernel is faster)
- kernel vs HE: 37.1x per query (HE timed on its first 40 seeds)
