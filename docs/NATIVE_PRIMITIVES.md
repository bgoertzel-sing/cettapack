# Native primitives for PeTTaChainer's frontier heaps and proof store, measured where the time goes

Task t-2026-11-26-cettapack-native-primitives (goal 2026-11-24-cettapack-roadmap; andy; M2, the loader, on hold).
Branch hive/t-2026-11-26-cettapack-native-primitives. **Started from the S1 branch head f5bac2a (main efcfdd3 + S0
f8126a1 + S1), not main**: neither S0 nor S1 is merged into main.

<!-- SUMMARY -->

## 1. Inputs and pins

| what | pin | note |
|---|---|---|
| cettapack | f5bac2a (S1 head = main efcfdd3 + S0 f8126a1 + S1) | this branch |
| CeTTa | a9b3a811 + upstream/cetta/0001-0004 + upstream/cetta-s0/0005 (files, unchanged) | as S0 |
| this task's patches (files) | upstream/cetta-np/0009 (profiler: Prolog-call timing), 0010 (native heaps) | apply with `git am` on 0001-0005; with S1's 0006/0007 also, with `git am --3way` (no conflicts) |
| SWI-Prolog | 10.0.2, user-local (scratch/deps/swipl10) | as S0 |
| PeTTaChainer | b0e24f9b (pettachainer/metta subtree), files unchanged | as S0 |
| PeTTa | e038e4db, lib/lib_*.metta overlaid | as S0 |
| workloads | S0's w1-w3, unchanged programs | bench/s0/gen_workloads.py, seed 20261124 |

Builds (bench/np/build_np_cetta.sh, trees under build/np; sha256 in evidence/np-2026-11-26/binaries.txt):
`prof` = 0001-0006 + 0009 + 0010, CETTA_FNPROF=1, -g (profiles, Prolog-call timing, samples); `on` = 0001-0005 + 0009 +
0010, CETTA_FNPROF=1 (S0's profiled configuration, used for both arms of the benchmark); `off` = the same with
CETTA_FNPROF=0 (profiler compiled out, for the profiler-off pair).


## 2. Do 1: where the time of the top rows goes (acceptance 1)

Method. The prof build (0001-0006 + 0009 + 0010) runs w1-w3 once, exactly as S0 (no shim), with three instruments:
S0's profiler rows (self time per function, calls); **0009**, new, which times every call from CeTTa into SWI-Prolog
(imported Prolog functions and callPredicate) and charges it to the current row and the predicate's name, with the
time spent converting terms CeTTa -> Prolog and back, and the heap size for heap calls; and S1's sampler (1 ms of CPU
per sample, native stacks), whose samples are split by native category (bench/s1/sample_categories.py) with samples
inside a Prolog call counted apart. Answers: all 13 file-runs byte-identical to S0's base stdout
(evidence/np-2026-11-26/do1/checks.md). Full tables: evidence/np-2026-11-26/do1/breakdown.md (bench/np/row_breakdown.py).

**Finding 1: CeTTa already has a native frontier heap.** CeTTa a9b3a811 answers `empty_heap`, `add_to_heap`,
`get_from_heap` and `heap_size` itself (src/petta_priority_heap.c: SWI library(heaps)' pairing heap on CeTTa's atoms,
ground finite heaps, on by default; `CETTA_PETTA_NATIVE_PRIORITY_HEAP=0` turns it off). In S0's runs none of these
calls reached Prolog (0009 counts 0 of them). The one heap operation that still crosses into SWI-Prolog is
PeTTaChainer's own `heap_pop` (heap_helpers.pl: `get_from_heap/4` packaged as `[P, X, Rest]`), an imported Prolog
function: each pop converts the whole heap term to Prolog and the answer back.

| workload | profiled s | all Prolog calls, % of run | heap_pop calls | heap_pop, % of run | of which term conversion | mean / max heap size at a pop | copy_term (Prolog), % of run | cyclic_term (callPredicate), % of run |
|---|---|---|---|---|---|---|---|---|
| w1-examples | 60.7 | 4.1 | 1,658 | **0.3** | 81% | 23 / 133 | 2.8 | 0.9 |
| w2-wmtm | 30.0 | 5.5 | 1,487 | **0.6** | 82% | 27 / 75 | 3.7 | 1.2 |
| w3-benchgen | 95.9 | 4.0 | 3,003 | **1.4** | 93% | 120 / 200 | 1.5 | 0.9 |

(All from evidence/np-2026-11-26/do1/breakdown.md, "Prolog calls by predicate". An earlier Do 1 run on the
previous build of the same profiler code gave the same figures within 0.1 point.)

**Finding 2: the top rows are interpretation, not heap or store operations.** Per row (self time, the row's own
Prolog calls, samples by native category; w1 shown, w2/w3 in breakdown.md):

| row (w1) | self % of run | ms/call | own Prolog calls, % of row | samples by native category (%) |
|---|---|---|---|---|
| advance-frontier-waiter-heap | 13.3 | 1.16 | copy_term 6.4 | matching 22, call prep 17, dispatch 14, alloc 13, profiler 9, copying 8, in Prolog 7, hashing 7 |
| spawn-frontier-goal-heap | 5.4 | 0.52 | none | matching 26, call prep 14, hashing 13, dispatch 13, copying 12, alloc 11 |
| search-agenda-heap (the pop loop) | 1.1 | 0.74 | heap_pop 28.9 | in Prolog 31, matching 20, alloc 14, prolog 13 |
| proof store (95 rows of backward_proof_store.metta) | 14.3 | 0.06 | none | call prep 26, matching 20, dispatch 14, alloc 14, profiler 10 |
| externalize-compiled-addition | 0.8 (w3: 7.1) | 1.9 (w3: 1.3) | none | matching 32, copying 18, dispatch 17, alloc 14 (w3 similar) |
| all rows named *heap* (41 rows) | 43.5 | 0.47 | copy_term etc. 3.2, heap_pop 0.7 | matching 23, call prep 16, dispatch 14, alloc 12, copying 11, hashing 8 |

- **advance-frontier-waiter-heap, spawn-frontier-goal-heap** (the order's top rows): no heap insert, pop or comparison
  happens in them at all. Their time is the interpretation of long let*/if/case bodies: equation selection and call
  preparation, matching (equation heads, case patterns, space reads of &goal_waiter etc.), goal dispatch, allocation,
  copying, hashing. Their only Prolog call is `copy_term` of the bundle (6-8% of the row). The heap insert happens
  in the callees (spawn-frontier-heap -> schedule-goal-heap -> add_to_heap), natively already.
- **The heap pop** is in search-agenda-heap: 29% / 43% / 61% of that row (w1/w2/w3), 79-93% of it converting the heap
  term. That is all a heap primitive can still remove: 0.3% / 0.6% / 1.4% of the runs.
- **Proof store** (rows defined in backward_proof_store.metta: 14.3% / 10.3% / 5.2% of w1/w2/w3): no Prolog calls;
  143,567 calls in w1 at 0.06 ms each. A lookup is `once (match &proof (stored-proof-row $pid ...))` wrapped in 2-4
  small functions (proof-output-key-by-id -> candidate-output-key -> proof-output-atom-by-id -> proof-output-atom).
  "Matching" (which includes equation-head and case matching, not only the &proof lookup) is at most 18-24% of the
  group; the rest is per-call interpretation. A native lookup primitive replaces only the match: bound 3% of w1, 2.5%
  of w2, 1% of w3, and less in practice since PeTTaChainer's wrappers stay. PeTTaChainer's equations also cannot be
  shadowed by a shim (an extra equation for an imported or defined head does not replace it: probe in section 3), so
  a store primitive would mean editing PeTTaChainer's files, which the order excludes.
- **externalize-compiled-addition** (w3's 7.1%): KB ingestion, a recursive walk over each compiled addition
  (externalize-proof-structure: is-var/is-expr tests, cons/car-atom/cdr-atom, one unification) run inline. Not a heap
  or store; a native version would be a term-rewriting primitive for one PeTTaChainer function.

Decision (the order's rule for this case): the heap primitive was plausibly worth building for the pop (small, exact,
cheap to verify) and is built and measured below; the proof-store primitive was **not** built: its ceiling is a few
percent at best and it would need PeTTaChainer edits. The larger Prolog costs, `copy_term` (1.5-3.7%) and `cyclic_term`
(0.9-1.2%), are not heap or store operations; they are noted as candidates (section 7).

## 3. The primitive: native heap_pop (upstream/cetta-np/0010) and the shim

- **Patch 0010** (petta_libpl.c only, +296 lines; opt-in): registers a foreign predicate `cettapack_native_heaps/1`
  (`on` / `check` / `off`) in CeTTa's private Prolog module. Until a program calls it, nothing changes. While on, a
  call of the imported function `heap_pop(H)` is answered by CeTTa's existing native heap, `petta_priority_heap_call`
  on `(get_from_heap H $P $X $Rest)`, packaged as `(P X Rest)` and bound to the result position: a variable, or a
  pattern of three distinct variables (PeTTaChainer's `(let ($key $item $rest) (heap_pop $agenda) ...)`). Anything
  else declines to SWI-Prolog unchanged: a heap CeTTa's native heap declines (holding variables, a key its standard
  order comparison does not cover, another shape), another result position, carried delayed goals. An empty heap has
  no answer, as get_from_heap fails. Ordering, ties and duplicates are therefore CeTTa's own native heap's, which
  mirrors SWI's meld/pairing (equal keys: the newer root wins, LIFO) and which S0's baseline already uses for every
  insert. `check` computes each pop both ways, answers SWI's and counts differences (field-by-field atom comparison
  of the value and of each bound variable); counts go to `$CETTA_NP_HEAPS_STATS`, the first differences to
  `$CETTA_NP_HEAPS_LOG`.
- **Patch 0009** (profiler only): the Prolog-call timing used in Do 1; compiled only with CETTA_FNPROF=1 and active
  only with `$CETTA_FNPROF_FOREIGN=1`.
- **The thin wrapper** (bench/np/shim/np_heaps.metta, one statement, prints nothing):
  `!(let $_ (callPredicate (Predicate (cettapack_native_heaps on))) (empty))`, run as the first file of the same CeTTa
  process: `cetta --lang petta ... np_heaps.metta examples/x.metta` (`--lang petta` consumes consecutive .metta files
  in one runtime). PeTTaChainer's files stay byte-identical at b0e24f9b. What it replaces: the SWI-Prolog execution
  of heap_helpers.pl's `heap_pop/2` (and the term conversion around it); nothing else. Why not MeTTa redefinitions:
  a probe showed that an equation `(= (add_to_heap $h $p $x) ...)` added after the import does not replace the
  imported Prolog function (the Prolog answer is still returned), so the switch has to be at CeTTa's foreign-call
  boundary.
- Applies with `git am` on a9b3a811 + 0001-0005; with S1's 0006 and 0007 also applied, both 0009 and 0010 apply with
  `git am --3way` (no conflicts; checked). Nothing was sent to Zar's CeTTa.

A first version of 0010 reimplemented the whole of library(heaps) before Finding 1 was known; it was replaced by the
version above, which only adds heap_pop on top of CeTTa's own heap. A first check-mode run of w1-w3 then showed every
real pop declining (the per-run route check refused all 13 file-runs): PeTTaChainer pops through a result pattern,
which that version did not handle; the bag tests had used a result variable only. Pattern support and a pattern bag
test (h7) were added; the failed run is kept out of the evidence (it measured nothing native).

## 4. Equivalence (acceptance 2)

Sanity gate (docs/EXPERIMENT_SANITY_GATE.md): the known answers are S0's recorded stdout of every workload file
(evidence/s0-2026-11-24/runs/base, run 1), and every run here goes through S0's exact command, image and generated
programs (bench/np/run_workloads.sh rebuilds them; the generated programs' sha256 are in each env file). Every
file-run writes a check line: exit status, stdout sha256 and whether it equals S0's, failed tests, and with a shim
the heap counters and a **route** verdict (on: heap_pop answered natively at least once; check: compared and 0
differences; off: never answered natively). A file-run passes only with all of them; the route verdict is the guard
against the silent failure that the shim does nothing (it caught exactly that once, section 3).

- **w1-w3 end to end, check mode** (evidence/np-2026-11-26/check/checks.md): 13 of 13 file-runs pass; every one of the
  **6,148 heap_pop calls** of w1-w3 was computed natively and by SWI-Prolog and the two answers were identical (0
  differences, 0 declines).
- **w1-w3 end to end, on and off**: every file-run of the benchmark (section 5) passes its check line; stdout
  byte-identical to S0's in all of them.
- **Bag tests** (bench/np/gen_heap_bagtests.py, seed 20261126; bench/np/run_bagtests.sh; evidence/np-2026-11-26/
  bagtest/summary.md): seven PeTTa programs, each importing PeTTaChainer
  as its examples do and running heap operation sequences through PeTTaChainer's own heap functions, each run in four
  modes on the `on` build: none (no shim: SWI's heap_pop, the reference), off, on (native) and check. Pass = exit 0
  everywhere, every case printed, stdout of off/on/check byte-identical to none, 0 differences in check mode, and
  heap_pop answered natively in on mode. Each case prints every pop's (P X) or an empty-pop marker, sizes, and the
  final heap term itself (its structure decides later tie order). h1: hand cases (7 equal keys: LIFO; duplicates;
  int/float equal-value ties and -0.0/0.0/0; pops of an empty heap; 30 rounds of interleaved add/add/pop; negative
  and tiny keys); h2/h3: 50 seeded random sequences (h3 tie-heavy); h4: 800 adds, 300 pops, 100 adds, 610 pops with
  items holding variables (CeTTa's native heap declines such heaps, so this tests the decline path); h5/h6: the same
  sizes and 20 random sequences with ground items (native throughout); h7: 12 heaps of 5-400 entries drained through
  PeTTaChainer's pattern form `(let ($key $item $rest) (heap_pop $h) ...)`. **7 of 7 pass; 4,208 pops answered
  natively, every one equal to SWI's** ("declined" counts heap calls that ran in SWI: pops of non-ground heaps and
  the adds CeTTa's own heap declines):

| test | cases | none exit | off = none | on = none | check = none | check: checked / mismatched | on: native / declined | verdict |
|---|---|---|---|---|---|---|---|---|
| h1_hand | 6 of 6 printed | 0/0/0/0 | yes | yes | yes | 100 / 0 | 100 / 0 | PASS |
| h2_random_small | 40 of 40 printed | 0/0/0/0 | yes | yes | yes | 475 / 0 | 475 / 818 | PASS |
| h3_random_ties | 10 of 10 printed | 0/0/0/0 | yes | yes | yes | 157 / 0 | 157 / 652 | PASS |
| h4_large_with_vars | 1 of 1 printed | 0/0/0/0 | yes | yes | yes | 10 / 0 | 10 / 1800 | PASS |
| h5_large_ground | 1 of 1 printed | 0/0/0/0 | yes | yes | yes | 910 / 0 | 910 / 0 | PASS |
| h6_random_ground | 20 of 20 printed | 0/0/0/0 | yes | yes | yes | 1166 / 0 | 1166 / 0 | PASS |
| h7_pattern_drain | 12 of 12 printed | 0/0/0/0 | yes | yes | yes | 1390 / 0 | 1390 / 0 | PASS |

## 5. Benchmark (acceptance 3)

<!-- BENCH -->

## 6. Interpretation decisions (dispatcher's, followed)

- Patch numbering: upstream/cetta-np/0009 (profiler timing) and 0010 (the primitive); S2 may use 0008. Opt-in, off by
  default: with the shim absent CeTTa behaves as before (the only difference is one registered foreign predicate
  nobody calls; the baseline arm runs the same binary).
- Thin wrapper: PeTTaChainer byte-identical at b0e24f9b; the switch is a shim file (section 3), with what it replaces
  documented. Where the hot rows' time sits was established first (section 2), and the time is not in heap or store
  operations; the primitive was built for the one piece that is (the pop), the proof-store primitive was not.
- Equivalence: ordering, ties and duplicates are those of CeTTa's existing native heap (the same code that already
  answers every insert in the baseline), checked against SWI on every real pop and in the bag tests, including tie
  order, int/float/-0.0 ties, duplicates, empty pops, interleaving, large heaps and the decline path.
- Benchmark: baseline = the same build with the shim absent (S0's configuration: S0's command line, profiler on, as
  S0 ran it); new = with the shim; interleaved per run, RUNS=5, every w1-w3 file-run; each batch under
  `flock /tmp/cettapack-bench.lock`, load average recorded at its start and end (ledger.txt); S2's worker was running
  (a pilot under the same lock) when the benchmark started and was not touched.

## 7. Limits

- Single host, shared (other services and the S2 worker; load average about 5 on 8 cores); timing ranges in section 5.
- Do 1's categories come from function-name patterns over native stacks (S1's classifier); "matching" mixes equation
  selection matching with space lookups, so the proof-store bound is loose (an upper bound).
- 0009's conversion split nests: a Prolog call that calls back into CeTTa and into Prolog again counts its inner
  conversions in both; heap_pop, copy_term and cyclic_term do not call back.
- The native heap_pop covers ground heaps (as CeTTa's native heap does); in these workloads every pop was covered
  (0 declines). Non-ground heaps keep the SWI path (bag tests h2-h4 exercise it).
- Candidates not built: a native `copy_term` (1.5-3.7% of the runs, 55-61% of it conversion) and a cheaper
  `cyclic_term` check (0.9-1.2%): both are CeTTa-side conversions around Prolog calls (Zar's code if proposed), and
  together still below what S1/S2 found per-call interpretation costs.

## 8. Reproduce

```
export GMP_PREFIX=<libgmp-dev root>/usr SWIPL_PKG_CONFIG_PATH=<swipl 10.0.2 root>/share/pkgconfig
bench/np/build_np_cetta.sh prof; bench/np/build_np_cetta.sh on; bench/np/build_np_cetta.sh off
BUILD=prof SHIM=none FOREIGN=1 bench/np/run_workloads.sh evidence/np-2026-11-26/do1          # Do 1
python3 bench/s1/analyze_samples.py symbolise ... ; python3 bench/np/row_breakdown.py ...       # (recompute script)
BUILD=on SHIM=check bench/np/run_workloads.sh evidence/np-2026-11-26/check                     # every pop compared
BUILD=on bench/np/run_bagtests.sh evidence/np-2026-11-26/bagtest
BUILD=on BASE_SHIM=none NEW_SHIM=off RUNS=3 bench/np/run_bench.sh evidence/np-2026-11-26/bench-aa   # base vs base
BUILD=on RUNS=5 bench/np/run_bench.sh evidence/np-2026-11-26/bench                               # base vs native
BUILD=off RUNS=1 bench/np/run_bench.sh evidence/np-2026-11-26/bench-off                          # profiler off
bash bench/np/recompute_np.sh      # every table above, byte for byte
```

## 9. Files

- upstream/cetta-np/0009-*.patch, 0010-*.patch; bench/np/shim/np_heaps{,_check,_off}.metta.
- bench/np/: build_np_cetta.sh, run_workloads.sh, run_bench.sh, run_bagtests.sh, gen_heap_bagtests.py,
  bagtest/common.inc, row_breakdown.py, summarize_checks.py, summarize_bagtests.py, summarize_bench.py, recompute_np.sh.
- evidence/np-2026-11-26/: do1/ (profiles, gzipped samples + SAMPLES.sha256, breakdown.md/json, checks.md, ledger),
  check/, bagtest/, bench/, bench-aa/, bench-off/ (raw stdout/stderr/time/exit/check per file-run, env files, ledgers,
  summaries), binaries.txt, make-test.log.
