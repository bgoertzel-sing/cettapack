## cpk-site-tissue_fanout

7 processes per mode, interleaved run by run.

### Probe 1: direct entry point (match-rows, upstream/cetta/0003)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| generic | 16,309.8 | 15,227.8-16,940.6 | 51.6 | 51.3-51.9 | 173,693 | 0.52x (0.45-0.58x, 0/7) | - |
| hosted-generic | 8,358.4 | 7,513.8-9,468.2 | 46.8 | 46.6-47.0 | 173,693 | - | site calls 1,400, kernel 0, fallback 0 |
| direct | 6,069.0 | 5,235.1-6,550.8 | 46.7 | 46.4-46.9 | 173,693 | 1.36x (1.22-1.80x, 7/7) | - |
| kernel | 8,984.9 | 8,144.2-10,102.2 | 571.0 | 570.8-571.1 | 173,693 | 0.92x (0.83-1.06x, 2/7) | site calls 1,400, kernel 1,400, fallback 0 |
| fallback | 19,200.1 | 18,597.9-19,879.3 | 58.6 | 58.4-58.8 | 173,693 | 0.43x (0.38-0.51x, 0/7) | site calls 1,400, kernel 0, fallback 1,400 |
| m-generic | 15,994.7 | 15,164.9-19,839.4 | 51.7 | 51.5-51.8 | 173,693 | 0.52x (0.40-0.62x, 0/7) | - |
| m-hosted-generic | 8,179.4 | 7,050.8-8,484.4 | 46.3 | 45.9-46.5 | 173,693 | 1.06x (0.90-1.23x, 4/7) | site calls 1,400, kernel 0, fallback 0 |

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
| nested | 15,495.0 | 14,646.6-16,197.8 | 47.3 | 47.1-47.7 | 173,693 | 0.52x (0.48-0.62x, 0/7) | - |
| generic | 16,309.8 | 15,227.8-16,940.6 | 51.6 | 51.3-51.9 | 173,693 | 0.52x (0.45-0.58x, 0/7) | - |
| hosted-generic | 8,358.4 | 7,513.8-9,468.2 | 46.8 | 46.6-47.0 | 173,693 | - | site calls 1,400, kernel 0, fallback 0 |
| kernel | 8,984.9 | 8,144.2-10,102.2 | 571.0 | 570.8-571.1 | 173,693 | 0.92x (0.83-1.06x, 2/7) | site calls 1,400, kernel 1,400, fallback 0 |
| fallback | 19,200.1 | 18,597.9-19,879.3 | 58.6 | 58.4-58.8 | 173,693 | 0.43x (0.38-0.51x, 0/7) | site calls 1,400, kernel 0, fallback 1,400 |

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
| fn-generic | 121,098.0 | 115,194.0-125,671.0 | 59.9 | 59.6-60.1 | 173,693 | 1.03x (0.98-1.08x, 5/7) | - |
| fn-hosted | 124,041.6 | 121,641.7-129,933.1 | 59.4 | 59.0-59.6 | 173,693 | - | site calls 1,400, kernel 0, fallback 0 |
| fn-kernel | 122,421.3 | 118,837.1-125,847.7 | 571.2 | 570.9-571.4 | 173,693 | 1.00x (0.98-1.09x, 4/7) | site calls 1,400, kernel 1,400, fallback 0 |
| fn-spec | 7,883.0 | 6,999.1-8,513.1 | 46.9 | 46.7-47.2 | 173,693 | 15.69x (14.94-17.74x, 7/7) | fnspec calls 1,400, specialised 1,400, residual 0 |
| fn-fallback | 122,096.4 | 116,710.7-140,500.9 | 59.4 | 59.2-59.7 | 173,693 | 1.01x (0.88-1.10x, 4/7) | fnspec calls 1,400, specialised 0, residual 1,400 |

fn-hosted is hosted-generic for a function: the site callable on CeTTa's matcher inside the MeTTa function.

Route (match backend and native storage before -> after the timed batches):

- fn-generic: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-hosted: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-kernel: native, rows+dense 128,576+0 -> 128,576+0
- fn-spec: native, rows+dense 82,177+46,399 -> 82,177+46,399
- fn-fallback: native, rows+dense 82,177+46,399 -> 82,177+46,399

