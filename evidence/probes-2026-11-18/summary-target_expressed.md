## cpk-site-target-expressed

7 processes per mode, interleaved run by run.

### Probe 1: direct entry point (match-rows, upstream/cetta/0003)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| generic | 31.1 | 28.5-35.4 | 42.9 | 42.7-43.1 | 1,653 | 0.94x (0.83-1.20x, 2/7) | - |
| hosted-generic | 29.4 | 26.9-34.9 | 42.4 | 42.4-42.7 | 1,653 | - | site calls 7,000, kernel 0, fallback 0 |
| direct | 12.2 | 11.2-13.4 | 42.3 | 42.2-42.6 | 1,653 | 2.40x (2.20-2.80x, 7/7) | - |
| kernel | 36.4 | 32.2-40.5 | 566.6 | 566.4-566.8 | 1,653 | 0.87x (0.73-1.00x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 56.9 | 51.5-65.3 | 60.3 | 60.0-60.4 | 1,653 | 0.52x (0.47-0.54x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |
| m-generic | 43.0 | 40.4-49.0 | 42.5 | 42.2-42.6 | 1,653 | 0.68x (0.57-0.85x, 0/7) | - |
| m-hosted-generic | 35.3 | 32.7-39.3 | 41.8 | 41.6-42.0 | 1,653 | 0.79x (0.75-1.07x, 1/7) | site calls 7,000, kernel 0, fallback 0 |

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
| nested | 32.3 | 29.6-35.0 | 43.2 | 43.0-43.5 | 1,653 | 0.95x (0.83-1.00x, 0/7) | - |
| generic | 31.1 | 28.5-35.4 | 42.9 | 42.7-43.1 | 1,653 | 0.94x (0.83-1.20x, 2/7) | - |
| hosted-generic | 29.4 | 26.9-34.9 | 42.4 | 42.4-42.7 | 1,653 | - | site calls 7,000, kernel 0, fallback 0 |
| kernel | 36.4 | 32.2-40.5 | 566.6 | 566.4-566.8 | 1,653 | 0.87x (0.73-1.00x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 56.9 | 51.5-65.3 | 60.3 | 60.0-60.4 | 1,653 | 0.52x (0.47-0.54x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |

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
| fn-generic | 231.4 | 217.8-241.3 | 42.9 | 42.7-43.1 | 1,620 | 1.09x (1.02-1.22x, 7/7) | - |
| fn-hosted | 257.5 | 235.5-281.8 | 43.0 | 42.8-43.1 | 1,620 | - | site calls 7,000, kernel 0, fallback 0 |
| fn-kernel | 259.5 | 235.6-286.5 | 566.6 | 566.4-566.8 | 1,620 | 0.98x (0.91-1.17x, 2/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fn-spec | 28.4 | 27.0-55.3 | 42.4 | 42.1-42.6 | 1,620 | 8.77x (5.00-9.93x, 7/7) | fnspec calls 7,000, specialised 7,000, residual 0 |
| fn-fallback | 266.2 | 256.2-290.6 | 43.0 | 42.7-43.1 | 1,620 | 0.96x (0.88-1.05x, 2/7) | fnspec calls 7,000, specialised 0, residual 7,000 |

fn-hosted is hosted-generic for a function: the site callable on CeTTa's matcher inside the MeTTa function.

Route (match backend and native storage before -> after the timed batches):

- fn-generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-hosted: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-kernel: native, rows+dense 128,576+0 -> 128,576+0
- fn-spec: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-fallback: native, rows+dense 82,177+46,399 -> 82,177+46,399

