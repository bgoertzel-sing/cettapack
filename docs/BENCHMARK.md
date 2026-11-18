# Benchmark (§17)

## After M1 (2026-11-17): CeTTa a9b3a811, bulk lookup, option-B sites

Task t-2026-11-17-cettapack-m1-numbers. Run:
`M1_HOST=<M1 tree>/build/cettapack-host RUNS=7 REPEAT=5 OUT=evidence/bench-2026-11-17 bench/run_bench.sh`
at cettapack `4bd2687` (clean tree), CeTTa `a9b3a811` + 0001 + 0002, BUILD=mork.
Same host and settings as M1: pop-os, i7-1165G7, 8 threads, 15 GiB, Ubuntu
22.04, gcc 11.4; RUNS=7 processes per mode, REPEAT=5 batches per process, the
process's median batch; modes interleaved run by run. The host is shared (load
average 2.0 to 3.5 during the run, `evidence/bench-2026-11-17/env.txt`). Raw
JSON: `evidence/bench-2026-11-17/runs-*.jsonl`; commands: `commands.txt`;
summaries with every per-call component: `summary-*.md`. A RUNS=1 REPEAT=1
pilot (`pilot-RUNS1-REPEAT1.log`) is not usable for numbers: a process's first
batch of generic CeTTa is 2-3x slower than its later ones (M1's raw data shows
the same), and with REPEAT=1 the first batch is all there is.

**M1 baseline rerun.** The M1 tree (`d2df1cc`, CeTTa `4c285a97` + M1's 0001,
the M1 build) ran interleaved in the same run as modes `m1-*`, on the same KB
and seeds. Its kernel/generic ratio is 0.85x here (M1 reported 0.94x on Nov 6;
generic CeTTa itself measured 39.8 us here against 47.8 us then).

All sites use the M1 KB (`bench/gen_kb.py --seed 20261106`, 128,576 atoms,
sha256 `8097f2f4...5439f5c`); only the seeds differ (`--query-kind`). Answers
per batch are identical in every mode of a site; every kernel process ran
every call on the kernel path (guard VALID, 0 fallbacks).

Per-query time: median over the 7 processes, and the min-max range over them.
"Paired" ratios compare run i of two modes (they ran back to back).

### Site 1, M1's site: `(, (target $rs $gene) (expressed $gene $tissue))`, seed $rs

200 seeds, 1,653 answers per batch.

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) |
|---|---|---|---|---|
| generic | 46.2 | 41.7-48.1 | 42.4 | 42.3-42.5 |
| hosted-generic | 34.6 | 31.9-47.1 | 41.8 | 41.6-42.1 |
| **kernel** (bulk narrowing, guard included) | **37.3** | 33.9-44.0 | 565.5 | 565.0-565.6 |
| kernel-cursor (M1's per-byte narrowing) | 46.7 | 45.4-50.8 | 565.5 | 565.1-565.5 |
| fallback | 64.0 | 59.0-71.5 | 59.1 | 58.9-59.3 |
| M1 rerun: generic (CeTTa 4c285a97) | 39.8 | 39.0-45.4 | 41.6 | 41.6-41.8 |
| M1 rerun: hosted-generic | 38.2 | 35.5-41.1 | 41.0 | 40.8-41.1 |
| M1 rerun: kernel | 47.0 | 42.8-52.3 | 563.1 | 562.7-563.1 |
| M1 rerun: fallback | 70.3 | 63.4-74.0 | 56.8 | 56.7-57.0 |

- Kernel vs generic CeTTa, same pin: **1.24x** (paired 1.16x, 1.07-1.28x, faster in 7/7 runs).
- Kernel vs generic CeTTa on M1's pin: 1.07x (paired 1.05x, 0.89-1.34x, 4/7 runs): a wash.
- Generic CeTTa got slower with the pin move on this site: 46.2 vs 39.8 us (paired 0.92x, 1/7 runs faster).
  Route check (docs/ROUTE_CHECK.md, 2026-11-17): both pins run this on CeTTa's native backend with native rows,
  through the same matcher with identical per-query counters; 35 stats-off reruns put the new pin at 0-4%
  slower (pooled paired median 1.03x), so the 14% gap of this session did not reproduce. No row here ran on a
  PathMap-only route; no numbers are changed.
- Kernel vs hosted-generic (CeTTa's matcher behind the same callable): 0.93x (paired 0.99x, 3/7).

### Site 2, gene_variants: `(, (target $rs $gene) (expressed $gene $tissue))`, seed $gene

Selective field not first: the seed is the second field of `target`. 200 gene
seeds, 7,377 answers per batch (about 37 per query).

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) |
|---|---|---|---|---|
| generic | 118.0 | 108.7-136.5 | 42.7 | 42.6-42.9 |
| hosted-generic | 78.3 | 62.7-86.7 | 41.9 | 41.7-42.0 |
| **kernel** | **85.0** | 71.4-93.8 | 565.1 | 564.9-565.2 |
| kernel-cursor | 102.6 | 91.8-187.4 | 565.1 | 564.7-565.2 |
| fallback | 176.8 | 148.9-182.1 | 58.6 | 58.5-58.8 |

- Kernel vs generic: **1.39x** (paired 1.46x, 1.29-1.55x, 7/7). Kernel vs hosted-generic: 0.92x (2/7).

### Site 3, tissue_fanout: `(, (expressed $gene $tissue) (target $rs $gene))`, seed $tissue

Selective field not first and large fan-out: about 4,300 answers per query.
40 tissue seeds (fewer seeds, because of the fan-out), 173,693 answers per batch.

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) |
|---|---|---|---|---|
| generic | 16,270 | 14,849-17,087 | 51.7 | 51.7-51.9 |
| hosted-generic | 7,780 | 7,154-8,184 | 46.6 | 46.4-46.7 |
| **kernel** | **9,292** | 8,362-9,455 | 570.9 | 570.5-571.1 |
| kernel-cursor | 10,822 | 10,399-11,356 | 570.8 | 570.7-571.0 |
| fallback | 19,817 | 18,974-22,981 | 58.5 | 58.3-58.6 |

- Kernel vs generic: **1.75x** (paired 1.72x, 1.58-2.04x, 7/7). Kernel vs hosted-generic: 0.84x (0/7).

### Site 4, variant_interactor: `(, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue))`, seed $rs

Three-way join; the joined gene is the second field of `interacts`. 200 seeds,
9,835 answers per batch (about 49 per query).

| mode | per query (us) | range (us) | peak RSS (MiB) | range (MiB) |
|---|---|---|---|---|
| generic | 181.4 | 167.3-228.7 | 44.6 | 44.2-44.8 |
| hosted-generic | 102.8 | 93.5-191.8 | 43.3 | 43.0-43.4 |
| **kernel** | **113.2** | 101.6-130.2 | 817.9 | 817.6-818.1 |
| kernel-cursor | 137.1 | 131.8-154.8 | 817.7 | 817.6-818.0 |
| fallback | 225.8 | 219.8-265.0 | 64.3 | 64.2-64.4 |

- Kernel vs generic: **1.60x** (paired 1.60x, 1.37-1.85x, 7/7). Kernel vs hosted-generic: 0.91x (2/7).

### Option A: bulk lookup (upstream/cetta 0002)

PathMap narrowing per kernel call, same binary, bulk call vs M1's per-byte
cursor calls (`CPK_NARROW=cursor`):

| site | bulk (ns/call) | per-byte cursor (ns/call) | ratio | candidates/call |
|---|---|---|---|---|
| target_expressed | 2,334 | 8,316 | 3.6x | 8.7 |
| gene_variants | 7,264 | 23,518 | 3.2x | 38.5 |
| tissue_fanout | 927,906 | 2,421,441 | 2.6x | 4,255 |
| variant_interactor | 12,377 | 37,251 | 3.0x | 47.1 |

On M1's site the bulk call alone moves the kernel from 0.99x generic (cursor)
to 1.24x. M1 measured 8,901 ns/call narrowing on the old pin.

### Answer: does any configuration beat generic CeTTa?

| site | kernel beats generic CeTTa (evaluator `match`), same pin? | kernel beats CeTTa's matcher behind the same callable (hosted-generic)? |
|---|---|---|
| target_expressed (M1) | **yes**, 1.24x (7/7 runs); against M1's pin a wash (1.07x, 4/7) | no (0.93x) |
| gene_variants | **yes**, 1.39x (7/7) | no (0.92x) |
| tissue_fanout | **yes**, 1.75x (7/7) | no (0.84x) |
| variant_interactor | **yes**, 1.60x (7/7) | no (0.91x) |

The kernel with bulk narrowing beats generic CeTTa end to end on all four
sites, guards included, with the same answers. But hosted-generic beats the
kernel on all four: CeTTa's own conjunction matcher, called from the same
hosted callable, without PathMap, without the index (no 0.5-0.75 s build, no
extra 520-775 MiB). So the gain over generic comes from hosting the site as a
callable (skipping the evaluator's per-query and per-answer work around
`match`), not from the compiled kernel's matching; that gain grows with the
answers per query (1.3x to 2.1x for hosted-generic). The harder sites did not
make CeTTa's matcher weaker relative to the kernel: kernel/hosted-generic stays
at 0.84-0.93x. Inside the kernel, rematch and answer construction now cost
more than narrowing on every site.

Caveats: hosted-generic's answers are checked by count per batch (equal in
every run), not bag by bag; the kernel's are checked bag by bag (`make test`).
One shared host, one KB family, synthetic data.

### Correctness on the new pin

`make test` (`evidence/tests-2026-11-17/make-test.log`): M1's 23 items, the HE
comparison (37/37 bags) and the cross-space refusal, then `difftest` on every
site: bag and support demand on four small spaces (duplicates, co-reference,
equal values in distinct variables, dangling joins, a 70-byte symbol that MORK
interns), every symbol of the space as a seed plus an absent one, then all
seeds of the site's query file on the benchmark KB (200 each): 9/9 per site,
every call on the kernel path. The same difftest with `CPK_NARROW=cursor`:
`evidence/tests-2026-11-17/difftest-cursor-narrowing.txt`.

What would change the result now: a kernel whose rematch and answer
construction beat CeTTa's matcher (today they are 3-4 us and 3 us per call on
M1's site), an index that does not cost half a gigabyte, or sites where
CeTTa's matcher scans (hosted-generic's times suggest it narrows all four of
these; its indexing was not inspected).

## M1 (2026-11-06): CeTTa 4c285a97, per-byte narrowing, one site

Run: `RUNS=7 REPEAT=5 bench/run_bench.sh`, kernel and host code at `29ff6fc`
(`evidence/bench/env.txt`; its modified paths were the evidence files and the
mode interleaving in `bench/run_bench.sh`, committed right after). Raw per-process JSON: `evidence/bench/runs-*.jsonl`;
exact commands: `evidence/bench/commands.txt`; host: `evidence/bench/env.txt`
(i7-1165G7, 8 threads, 15 GiB, Ubuntu 22.04, gcc 11.4). The host is shared:
load average was about 3.5 during the run (another agent's CeTTa build was
running), so the modes are interleaved run by run and every number is a median
over 7 processes. Each process times 5 batches of all seeds and reports its
median batch. Per-call component times are means over a process's calls, then
the median over processes. Peak memory is the process's `ru_maxrss`.

### Workload

`bench/gen_kb.py --seed 20261106` (defaults): 128,576 atoms (118,845 distinct),
sha256 `8097f2f4ad2990cdf096cdcf428cf719397f4695536b6f51416fe9e3d5439f5c`:
`(target rsN geneM)` (45,346 records), `(expressed geneM tissueK)` (29,053),
`(interacts ...)` (32,522) and `(annotated ...)` (21,655); about 6% of facts
are written two or three times. 200 seeds, 5% of them absent; 8.3 answers per
query on average. `(target rsN $gene)` is the selective grounded prefix: 1 to 7
records out of 45,346.

### Modes (§17 baselines)

| Mode | What one query runs |
|---|---|
| generic | generic CeTTa: `(match <space> (, (target rsN $gene) (expressed $gene $tissue)) ($gene $tissue))` via `cetta_embed_eval` |
| hosted-generic | the site callable answering with CeTTa's generic conjunction matcher (`space_query_conjunction`) and the same answer construction: isolates the matcher |
| kernel | the site callable: **guard on every call**, then the kernel |
| fallback (residual) | the site callable with PathMap disabled: guard, INVALID, residual on every call |
| he | HE 0.2.10 (`hyperon`): `!(match &self ...)` |

PeTTa: not run (no SWI-Prolog on this host, no root). Raw MM2/MORK path: not
relevant to a single `match` site.

### Main KB (128,576 atoms, 200 seeds)

| | generic | hosted-generic | **kernel (guards included)** | fallback |
|---|---|---|---|---|
| per query (us) | **47.8** | 38.1 | **51.1** | 69.7 |
| fast-path guard latency (ns/call) | - | - | 142 | 169 |
| PathMap narrowing (ns/call) | - | - | 8,901 | - |
| native rematch (ns/call) | - | - | 4,820 | - |
| multiplicity reconstruction (ns/call) | - | - | 164 | - |
| answer construction (ns/call) | - | - | 3,493 | - |
| fallback (ns/call) | - | - | - | 38,795 |
| fallback frequency | - | - | 0 | 1.0 |
| PathMap candidates / call | - | - | 8.66 | - |
| native rematches / call | - | - | 8.66 | - |
| source records / call | - | - | 9.25 | - |
| index build, once (ms) | - | - | 489 | - |
| peak RSS (MiB) | 42.4 | 41.8 | 564.2 | 57.5 |

Compile-time cost: `tools/cpkc.py` 41 ms (median of 5,
`evidence/bench/compile-cost.txt`), plus the 489 ms index build at
instantiation.

Answers per batch are 1,653 in every mode; the bags are compared in
`make test` (items 15 and 16).

- **Kernel vs generic CeTTa, end to end, guards included: 0.94x** (the kernel
  is about 7% slower per query).
- Inside the callable: kernel 21.1 us + guard 0.14 us, against CeTTa's own
  matcher 10.9 us on the same path: the kernel's matching is about 2x slower
  than CeTTa's matcher on this site.
- Guard: 0.14 us per call, 0.3% of a query.
- Residual fallback: 1.46x generic.
- Amortization break-even: none on this site, since the kernel never beats the
  generic path; the 0.5 s instantiation is not recovered.

The first, pre-note version rematched every source record (10.0 rematches and
6.0 us per call); rematching once per candidate and multiplying record counts
(§10.1) brought rematch to 8.66 per call and 4.8 us, and the end-to-end ratio
from 0.83x to 0.94x.

### HE-scale KB (1,264 atoms, 40 seeds)

HE 0.2.10 aborts with a Rust panic (`hyperon-space/src/index/trie.rs:179`) on
this KB family from about 3,000 atoms up, the main KB included; reproducer in
`evidence/bench/he-crash.txt`. So HE was compared on a 1,264-atom KB from the
same generator and seed.

| | generic CeTTa | kernel (guards included) | HE 0.2.10 |
|---|---|---|---|
| per query (us) | 40.8 | 45.7 | 1,698.7 |
| answers / batch | 371 | 371 | 371 |
| peak RSS (MiB) | 11.5 | 16.4 | 29.4 |

`make test` item 16 checks the bags themselves: 37 of 37 distinct seeds give
the same bag in the kernel and in HE.

### What this says

On this site and KB, the guarded kernel is correct but not faster than generic
CeTTa. CeTTa's matcher already narrows this pattern with its own indexes in
about 11 us. The kernel spends 8.9 us in PathMap narrowing alone, because each
trie descent is a series of FFI calls into the bridge (one per key byte and per
value, one allocation per key read), plus 4.8 us of rematch and 3.5 us copying
answers. Most of a generic query's 48 us is evaluation around the matcher,
which a kernel at this boundary cannot remove.

What would change the result (not done in M1):

1. A bulk bridge call that descends a prefix and returns the values below it
   in one call would remove most of the narrowing time.
2. Sites where CeTTa's generic path scans more: a selective field that is not
   first, larger fan-out, multi-space joins (M2+).
