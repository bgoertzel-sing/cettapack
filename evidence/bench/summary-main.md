| mode | processes | queries/batch | answers/batch | per-query median (us) | guard ns/call | narrow ns/call | rematch ns/call | multiplicity ns/call | answer ns/call | fallback ns/call | fallback freq | candidates/call | rematches/call | records/call | index build (ms) | KB load (ms) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| generic | 7 | 200 | 1653 | 47.83 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.000 | 0.000 | 0.0 | 1193.0 | 42.4 |
| hosted-generic | 7 | 200 | 1653 | 38.12 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.000 | 0.000 | 0.000 | 0.000 | 0.0 | 1192.3 | 41.8 |
| kernel | 7 | 200 | 1653 | 51.14 | 142.1 | 8900.5 | 4819.8 | 163.5 | 3493.4 | 0.0 | 0.000 | 8.655 | 8.655 | 9.250 | 489.1 | 1181.8 | 564.2 |
| fallback | 7 | 200 | 1653 | 69.69 | 169.2 | 0.0 | 0.0 | 0.0 | 0.0 | 38795.2 | 1.000 | 0.000 | 0.000 | 0.000 | 150.1 | 1179.1 | 57.5 |

- kernel (guards included) vs generic CeTTa, end to end: 0.94x per query (>1 means the kernel is faster)
- kernel vs CeTTa's generic matcher behind the same callable: 0.75x per query
- in-callable work per call: kernel 21.12 us + guard 0.14 us vs generic matcher 10.89 us
- forced fallback vs generic CeTTa: 1.46x per query (guard + residual)
