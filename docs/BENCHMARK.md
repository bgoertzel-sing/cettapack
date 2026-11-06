# Benchmark (§17, adapted)

Run: `RUNS=5 REPEAT=5 bench/run_bench.sh` at cettapack `1f44464` (the two
modified paths at run time were the regenerated `certificate.json` and test
log, not code). Raw per-process JSON: `evidence/bench/runs-*.jsonl`; exact
commands: `evidence/bench/commands.txt`; host: `evidence/bench/env.txt`
(i7-1165G7, 8 threads, 15 GiB, Ubuntu 22.04, gcc 11.4); KB digests:
`evidence/bench/kb-*.txt`.

Every number is a median over 5 separate processes; each process times 5
batches of all seeds and reports its median batch. Per-call component times are
means over all calls of a process (then the median over processes). Peak memory
is the process's `ru_maxrss`.

## Workload

`bench/gen_kb.py --seed 20261106` (defaults): 128,576 atoms (118,845 distinct;
about 6% of facts written 2-3 times), sha256
`8097f2f4ad2990cdf096cdcf428cf719397f4695536b6f51416fe9e3d5439f5c`:
`(target rsN geneM)` (45,346 records), `(expressed geneM tissueK)` (29,053),
`(interacts ...)` (32,522) and `(annotated ...)` (21,655). 200 seeds `rsN`, 5% of
them absent. Each query is the site with one seed; on average 8.3 answers.
`(target rsN $gene)` is the selective grounded prefix: 1 to 7 records (1-3
distinct genes plus duplicates) out of 45,346 target records.

## Modes

| Mode | What one query runs |
|---|---|
| generic | generic CeTTa: `(match <space> (, (target rsN $gene) (expressed $gene $tissue)) ($gene $tissue))` through `cetta_embed_eval` |
| hosted-generic | `(cpk-site-target-expressed <space> rsN)` where the callable answers with CeTTa's generic conjunction matcher (`space_query_conjunction`) and the same answer construction as the kernel: isolates the matcher |
| kernel | `(cpk-site-target-expressed <space> rsN)`: **guard on every call**, then the kernel |
| fallback | the same call with PathMap narrowing disabled: guard on every call -> INVALID -> residual (generic `match`) |
| he | HE 0.2.10 (`hyperon` from PyPI): `!(match &self (, ...) ...)` via `MeTTa.run` |

## Results: main KB (128,576 atoms, 200 seeds)

| mode | per query (us) | guard (ns/call) | narrowing (ns/call) | rematch (ns/call) | answer construction (ns/call) | fallback (ns/call) | candidates/call | rematches/call | index build (ms, once) | peak RSS (MiB) |
|---|---|---|---|---|---|---|---|---|---|---|
| generic | **49.0** | - | - | - | - | - | - | - | - | 42.3 |
| hosted-generic | 40.3 | - | - | - | - | - | - | - | - | 41.4 |
| kernel (guards included) | **58.9** | 150 | 11,605 | 6,001 | 4,847 | - | 9.40 | 10.03 | 567 | 563.2 |
| fallback | 77.1 | 156 | - | - | - | 43,083 | - | - | 163 | 56.8 |

Answers per batch are 1,653 in every mode (the bags themselves are compared in
`make test`, item 15). Narrowing runs 2.76 trie descents per call (one for the
seed, one per joined gene). 9.40 candidate atoms lead to 10.03 rematched
records (duplicates), and every rematch succeeds: the trie prefix is exact for
this site, so rematch confirms rather than filters.

- **Kernel vs generic CeTTa, end to end, guards included: 0.83x** (the kernel
  is about 20% slower per query).
- In-callable work per call: kernel 28.0 us (guard 0.15 us) against CeTTa's
  generic matcher 10.7 us on the same hosted path: **the kernel's matching is
  about 2.6x slower than CeTTa's own matcher** on this site.
- Guard cost is 0.15 us per call, about 0.3% of a query.
- Forced fallback costs 1.57x generic (guard + building and evaluating the
  residual expression).
- Memory: the PathMap/MORK narrowing index takes the kernel process from 42 MiB
  to 563 MiB, and 0.57 s to build once.

## Results: HE-scale KB (1,264 atoms, 40 seeds)

HE 0.2.10 aborts with a Rust panic (`hyperon-space/src/index/trie.rs:179`,
`Option::unwrap()` on `None`) on this KB family from about 3,000 atoms up,
including the main KB; reproducer in `evidence/bench/he-crash.txt`. So HE was
compared on a 1,264-atom KB from the same generator and seed
(`--snps 240 --genes 60 --interactions 300 --annotations 200`).

| mode | per query (us) | answers/batch | peak RSS (MiB) |
|---|---|---|---|
| generic CeTTa | 46.1 | 371 | 11.2 |
| kernel (guards included) | 49.2 | 371 | 16.3 |
| HE 0.2.10 | 2,589 | 371 | 29.5 |

HE returns the same number of answers. Both CeTTa paths are about 50x faster
than HE here; the kernel is again slightly slower than generic CeTTa (0.94x).

PeTTa was not run: it needs SWI-Prolog, which this host does not have and I
did not install (no root).

## What this says

On this site and KB, a guarded PathMap-narrowed kernel hosted in CeTTa is
**correct but not faster** than generic CeTTa. CeTTa's own matcher already
narrows this pattern with its native indexes and costs about 11 us per call;
the kernel's narrowing alone costs about 12 us, because each trie descent is a
sequence of FFI calls into the bridge (one per key byte, one per value, one
allocation per key read), and its per-record rematch and answer copy add
another 11 us. Where the time goes in the generic query (about 49 us) is
mostly around the matcher, in evaluation, which a kernel at this boundary
cannot remove.

What would change the result (not done in M1):

1. A bulk narrowing call in the bridge (descend a whole prefix and return the
   group ids below it in one call) would remove most of the 11.6 us.
2. Rematching each distinct candidate once and replaying the success for its
   duplicate records would halve rematch on duplicate-heavy data, keeping
   multiplicity from the records; it needs care that rematch has no
   record-specific effect (it has none for ground records).
3. Kernels pay off when narrowing is the bottleneck: sites whose selective
   field is not the first argument, multi-space joins, or larger fan-out,
   where CeTTa's generic path scans more. This site was chosen because the
   order suggested it, and CeTTa already handles its shape well.
