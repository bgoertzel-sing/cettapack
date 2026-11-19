## cpk-site-gene_variants

7 processes per mode, interleaved run by run.

### Probe 1: direct entry point (match-rows, upstream/cetta/0003)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| generic | 107.2 | 102.4-205.1 | 43.0 | 42.8-43.2 | 7,377 | 0.59x (0.33-0.65x, 0/7) | - |
| hosted-generic | 66.6 | 60.2-73.7 | 42.6 | 42.4-42.8 | 7,377 | - | site calls 7,000, kernel 0, fallback 0 |
| direct | 35.9 | 33.8-41.9 | 42.3 | 42.2-42.5 | 7,377 | 1.77x (1.52-2.18x, 7/7) | - |
| kernel | 75.6 | 72.1-143.8 | 566.2 | 566.0-566.4 | 7,377 | 0.84x (0.42-0.98x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 150.9 | 148.8-175.4 | 59.5 | 59.3-59.7 | 7,377 | 0.43x (0.37-0.49x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |
| m-generic | 120.9 | 114.2-132.9 | 42.0 | 41.8-42.1 | 7,377 | 0.56x (0.45-0.57x, 0/7) | - |
| m-hosted-generic | 71.4 | 66.5-82.7 | 41.2 | 41.0-41.5 | 7,377 | 0.87x (0.80-1.11x, 1/7) | site calls 7,000, kernel 0, fallback 0 |

m-generic / m-hosted-generic: the same modes measured by cettapack-host bench, the docs/BENCHMARK.md method (query atoms built inside the timed loop; generic parses the whole match text per query).

Route (match backend and native storage before -> after the timed batches):

- generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- hosted-generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- direct: native, rows+dense 82,177+46,399 -> 82,177+46,399
- kernel: native, rows+dense 128,576+0 -> 128,576+0
- fallback: native, rows+dense 128,576+0 -> 128,576+0
- m-generic: not recorded (cettapack-host bench)
- m-hosted-generic: not recorded (cettapack-host bench)

### Probe 2: fusing several matches (nested matches vs one fused kernel)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| nested | 114.8 | 107.0-124.6 | 43.1 | 43.0-43.4 | 7,377 | 0.57x (0.48-0.68x, 0/7) | - |
| generic | 107.2 | 102.4-205.1 | 43.0 | 42.8-43.2 | 7,377 | 0.59x (0.33-0.65x, 0/7) | - |
| hosted-generic | 66.6 | 60.2-73.7 | 42.6 | 42.4-42.8 | 7,377 | - | site calls 7,000, kernel 0, fallback 0 |
| kernel | 75.6 | 72.1-143.8 | 566.2 | 566.0-566.4 | 7,377 | 0.84x (0.42-0.98x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 150.9 | 148.8-175.4 | 59.5 | 59.3-59.7 | 7,377 | 0.43x (0.37-0.49x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |

nested: the same matches written one inside the other, run by CeTTa; kernel: the compiled kernel of the fused conjunction.

Route (match backend and native storage before -> after the timed batches):

- nested: native, rows+dense 82,177+46,399 -> 82,177+46,399
- generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- hosted-generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- kernel: native, rows+dense 128,576+0 -> 128,576+0
- fallback: native, rows+dense 128,576+0 -> 128,576+0

### Probe 3: specialising a whole function

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs fn-hosted: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| fn-generic | 952.5 | 901.3-1,039.5 | 43.3 | 43.0-43.5 | 7,268 | 1.13x (1.05-1.16x, 7/7) | - |
| fn-hosted | 1,061.3 | 976.6-1,106.7 | 43.1 | 42.8-43.4 | 7,268 | - | site calls 7,000, kernel 0, fallback 0 |
| fn-kernel | 1,040.9 | 984.8-1,099.0 | 566.1 | 565.8-566.4 | 7,268 | 1.03x (0.94-1.05x, 5/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fn-spec | 66.9 | 63.4-76.2 | 42.7 | 42.5-43.0 | 7,268 | 15.41x (13.55-17.41x, 7/7) | fnspec calls 7,000, specialised 7,000, residual 0 |
| fn-fallback | 1,003.6 | 960.6-1,143.9 | 43.7 | 43.5-43.9 | 7,268 | 1.06x (0.90-1.12x, 5/7) | fnspec calls 7,000, specialised 0, residual 7,000 |

fn-hosted is hosted-generic for a function: the site callable on CeTTa's matcher inside the MeTTa function.

Route (match backend and native storage before -> after the timed batches):

- fn-generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-hosted: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-kernel: native, rows+dense 128,576+0 -> 128,576+0
- fn-spec: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-fallback: native, rows+dense 82,177+46,399 -> 82,177+46,399

