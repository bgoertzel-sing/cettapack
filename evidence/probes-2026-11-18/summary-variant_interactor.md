## cpk-site-variant_interactor

7 processes per mode, interleaved run by run.

### Probe 1: direct entry point (match-rows, upstream/cetta/0003)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| generic | 171.4 | 159.9-183.9 | 45.1 | 44.8-45.1 | 9,835 | 0.52x (0.47-0.55x, 0/7) | - |
| hosted-generic | 86.4 | 82.7-95.1 | 44.2 | 44.1-44.4 | 9,835 | - | site calls 7,000, kernel 0, fallback 0 |
| direct | 62.7 | 57.6-72.3 | 44.1 | 44.0-44.5 | 9,835 | 1.40x (1.16-1.52x, 7/7) | - |
| kernel | 112.9 | 101.1-115.5 | 819.2 | 818.9-819.3 | 9,835 | 0.75x (0.72-0.92x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 213.3 | 200.6-241.9 | 65.7 | 65.5-65.8 | 9,835 | 0.41x (0.38-0.44x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |
| m-generic | 178.1 | 158.3-201.6 | 44.8 | 44.5-45.1 | 9,835 | 0.46x (0.42-0.57x, 0/7) | - |
| m-hosted-generic | 101.9 | 85.5-113.9 | 43.5 | 43.3-43.6 | 9,835 | 0.89x (0.74-0.98x, 0/7) | site calls 7,000, kernel 0, fallback 0 |

m-generic / m-hosted-generic: the same modes measured by cettapack-host bench, the docs/BENCHMARK.md method (query atoms built inside the timed loop; generic parses the whole match text per query).

Route (match backend and native storage before -> after the timed batches):

- generic: native, rows+dense 112,164+16,412 -> 112,164+16,412
- hosted-generic: native, rows+dense 112,164+16,412 -> 112,164+16,412
- direct: native, rows+dense 112,164+16,412 -> 112,164+16,412
- kernel: native, rows+dense 128,576+0 -> 128,576+0
- fallback: native, rows+dense 128,576+0 -> 128,576+0
- m-generic: not recorded (cettapack-host bench)
- m-hosted-generic: not recorded (cettapack-host bench)

### Probe 2: fusing several matches (nested matches vs one fused kernel)

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs hosted-generic: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| nested | 195.3 | 172.9-231.8 | 45.1 | 44.9-45.3 | 9,835 | 0.46x (0.38-0.49x, 0/7) | - |
| generic | 171.4 | 159.9-183.9 | 45.1 | 44.8-45.1 | 9,835 | 0.52x (0.47-0.55x, 0/7) | - |
| hosted-generic | 86.4 | 82.7-95.1 | 44.2 | 44.1-44.4 | 9,835 | - | site calls 7,000, kernel 0, fallback 0 |
| kernel | 112.9 | 101.1-115.5 | 819.2 | 818.9-819.3 | 9,835 | 0.75x (0.72-0.92x, 0/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fallback | 213.3 | 200.6-241.9 | 65.7 | 65.5-65.8 | 9,835 | 0.41x (0.38-0.44x, 0/7) | site calls 7,000, kernel 0, fallback 7,000 |

nested: the same matches written one inside the other, run by CeTTa; kernel: the compiled kernel of the fused conjunction.

Route (match backend and native storage before -> after the timed batches):

- nested: native, rows+dense 112,164+16,412 -> 112,164+16,412
- generic: native, rows+dense 112,164+16,412 -> 112,164+16,412
- hosted-generic: native, rows+dense 112,164+16,412 -> 112,164+16,412
- kernel: native, rows+dense 128,576+0 -> 128,576+0
- fallback: native, rows+dense 128,576+0 -> 128,576+0

### Probe 3: specialising a whole function

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) | answers/batch | vs fn-hosted: paired median (range, faster runs) | path (all runs) |
|---|---|---|---|---|---|---|---|
| fn-generic | 1,310.2 | 1,269.4-1,448.4 | 45.2 | 45.0-45.5 | 9,643 | 1.06x (0.92-1.14x, 4/7) | - |
| fn-hosted | 1,385.1 | 1,330.5-1,491.8 | 45.3 | 44.9-45.4 | 9,643 | - | site calls 7,000, kernel 0, fallback 0 |
| fn-kernel | 1,402.5 | 1,270.3-1,477.5 | 819.2 | 819.0-819.4 | 9,643 | 1.02x (0.90-1.09x, 4/7) | site calls 7,000, kernel 7,000, fallback 0 |
| fn-spec | 98.2 | 94.7-114.3 | 44.3 | 44.0-44.3 | 9,643 | 14.07x (11.64-15.34x, 7/7) | fnspec calls 7,000, specialised 7,000, residual 0 |
| fn-fallback | 1,403.8 | 1,323.5-1,492.0 | 45.7 | 45.1-45.8 | 9,643 | 0.97x (0.93-1.10x, 2/7) | fnspec calls 7,000, specialised 0, residual 7,000 |

fn-hosted is hosted-generic for a function: the site callable on CeTTa's matcher inside the MeTTa function.

Route (match backend and native storage before -> after the timed batches):

- fn-generic: native, rows+dense 112,164+16,412 -> 112,164+16,412
- fn-hosted: native, rows+dense 112,164+16,412 -> 112,164+16,412
- fn-kernel: native, rows+dense 128,576+0 -> 128,576+0
- fn-spec: native, rows+dense 112,164+16,412 -> 112,164+16,412
- fn-fallback: native, rows+dense 112,164+16,412 -> 112,164+16,412

