# Benchmark (§17)

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

## Workload

`bench/gen_kb.py --seed 20261106` (defaults): 128,576 atoms (118,845 distinct),
sha256 `8097f2f4ad2990cdf096cdcf428cf719397f4695536b6f51416fe9e3d5439f5c`:
`(target rsN geneM)` (45,346 records), `(expressed geneM tissueK)` (29,053),
`(interacts ...)` (32,522) and `(annotated ...)` (21,655); about 6% of facts
are written two or three times. 200 seeds, 5% of them absent; 8.3 answers per
query on average. `(target rsN $gene)` is the selective grounded prefix: 1 to 7
records out of 45,346.

## Modes (§17 baselines)

| Mode | What one query runs |
|---|---|
| generic | generic CeTTa: `(match <space> (, (target rsN $gene) (expressed $gene $tissue)) ($gene $tissue))` via `cetta_embed_eval` |
| hosted-generic | the site callable answering with CeTTa's generic conjunction matcher (`space_query_conjunction`) and the same answer construction: isolates the matcher |
| kernel | the site callable: **guard on every call**, then the kernel |
| fallback (residual) | the site callable with PathMap disabled: guard, INVALID, residual on every call |
| he | HE 0.2.10 (`hyperon`): `!(match &self ...)` |

PeTTa: not run (no SWI-Prolog on this host, no root). Raw MM2/MORK path: not
relevant to a single `match` site.

## Main KB (128,576 atoms, 200 seeds)

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

## HE-scale KB (1,264 atoms, 40 seeds)

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

## What this says

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
