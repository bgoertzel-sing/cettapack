# S0 (Ben's Milestone 0): profiling real PeTTaChainer runs on CeTTa

Task t-2026-11-24-cettapack-s0-profile (goal 2026-11-24-cettapack-roadmap, stage S0; andy, Work msg 840;
order projects/cettapack/orders/2026-11-24-cettapack-s0-profile.md). Ben's note
docs/cetta-guarded-supercompilation-2026-07-06.pdf, section 20, Milestone 0: "profiling for hot regions, temporal
stability, space dependency vectors, effect summaries, candidate counts, result counts, and guard overhead".

## Summary

**Question.** On real PeTTaChainer runs on CeTTa, which functions take the time, how much of it do the shapes fnspec
compiles so far cover, and what would S1-S3 have to compile or guard?

**Answer.** Very little is covered, and most of the time is out of fnspec's reach. On three workloads (PeTTaChainer's
own 11 examples, a wmtm-style reasoning run on wmtm's synthetic fixture vocabulary, and a 3,957-statement benchgen
KB), functions or match sites that the fnspec compiler accepts as written hold **0.21%, 0.31% and 0.17%** of the
self time. Every extension the compiler's refusals point to, added together, would bring that to **2.5%, 8.5% and
14.3%**; the two that matter are single hot match sites (w2: a match on the bare-symbol space ccls_index with a
unification test in its condition, 5.9%; w3: an =alpha test against quoted tuples, 5.3%). 77-93% of the self time is in
rows where fnspec has nothing to compile (no match in the body, or the code around a match):
PeTTaChainer's frontier/agenda heaps and proof-store bookkeeping
(advance-frontier-waiter-heap alone is 13% of the examples) and, for the large KB, ingestion
(externalize/internalize-runtime-addition). For S2/S3: definitions (&self) change only between top-level statements
(once per KB update, not per call); the search-state spaces the hot rows read are rewritten constantly (30-50% of
their publications remove atoms), so guards on them fail and they are not append-only; the KB-side indexes are
append-only during ingestion (S3's case), except that queries add and erase temporary rules in them.

**Open / for the reader.** (1) The order's PeTTaChainer pin, 33cca343, does not run on CeTTa (queries answer ()); the
profiles are of **b0e24f9b**, the revision CeTTa's own compatibility harness pins, which passes that harness 3/3 here.
(2) PeTTaChainer needs CeTTa's SWI-Prolog interop at all (callPredicate, assertz/erase, heap helpers): without it
every query answers (); with it CeTTa needs SWI-Prolog >= 9.3's API, so SWI-Prolog 10.0.2 was built from source,
user-locally. cettapack's own CeTTa build (make test) still has no Prolog. (3) Queries inside CeTTa's prepared or
inlined calls are not separated from the code around them (only 8-16% of space accesses happen in match-site rows),
so the coverage figures are lower bounds; the loose upper bound (rows that touch a space at all) is 61-70%.
Instrumentation off: answers byte-identical on every file and run, and the off binary's machine code is the
uninstrumented one's apart from 67 line-number immediates; on: about 4-10% slower (noisy shared host).

## 1. Inputs and pins

| what | pin | note |
|---|---|---|
| cettapack | main efcfdd3 (PR 7, fnspec 3 merged) | branch hive/t-2026-11-24-cettapack-s0-profile |
| CeTTa | a9b3a811 + upstream/cetta/0001-0004 (files, unchanged) | as docs/PINS.md |
| profiler | upstream/cetta-s0/0005-fnprof-...patch (file) | applies on 0001-0004; opt-in build flag CETTA_FNPROF=1 |
| SWI-Prolog | 10.0.2 from source (swipl-10.0.2.tar.gz sha256 e42cc098...4b57), user-local | CeTTa's Prolog interop (ENABLE_LIB_PROLOG=1) |
| PeTTaChainer | **b0e24f9b** (pettachainer/metta subtree) | see 1.1 |
| PeTTa | e038e4db, lib/lib_*.metta overlaid | as CeTTa's PeTTaChainer compatibility harness |
| wmtm | 9a8f693, tests/fixtures/synthetic/wmtm_state.json (sha256 bf85b582...7875) | vocabulary only |

### 1.1 Which PeTTaChainer, and why CeTTa needs SWI-Prolog for it

The order names the PeTTaChainer pin the fnspec sites used, **33cca343** (2026-11-16). On CeTTa it does not run:

1. PeTTaChainer calls Prolog: `callPredicate` (assertz/erase for its temporary query clauses, cyclic_term checks),
   `import_prolog_function` (the heap helpers of its agenda), `import_prolog_functions_from_file`. CeTTa provides
   these only when built with its SWI-Prolog interop (`ENABLE_LIB_PROLOG`, on when pkg-config finds swipl). This
   host had no SWI-Prolog, so cettapack's CeTTa build (and the fnspec work) had it off; without it every
   `callPredicate` stays an unevaluated expression, nothing reaches the chainer's stores, and every query answers
   (). That holds for both pins and for CeTTa's own PeTTaChainer compatibility harness (`make
   test-petta-chainer-compat`, scripts/petta_chainer_compat.py, manifest tests/petta/chainer_compat/manifest.json
   at a9b3a811, which pins PeTTaChainer **b0e24f9b** (2026-08-10) and PeTTa e038e4db): 3/3 examples fail.
2. CeTTa's interop needs the SWI-Prolog 9.3.x+ foreign API (PL_S_YIELD_DEBUG, PL_CLEANUP_NO_CANCEL): Ubuntu 22.04's
   8.4.2 and the stable 9.2.9 do not compile it. SWI-Prolog 10.0.2 was built from source, user-locally (no
   sudo; scratch/deps/swipl10, without the X/Java/ODBC/Python/terminal packages). With it, CeTTa a9b3a811 +
   0001-0004 + ENABLE_LIB_PROLOG=1 passes CeTTa's compatibility harness **3/3** on b0e24f9b
   (evidence/s0-2026-11-24/compat/b0e24f9-base-summary.json; each example's normalised answers match the
   manifest's digest).
3. 33cca343 still answers () on that build (evidence/s0-2026-11-24/compat/33cca343-deductionrevision-base.out:
   `is (), should ...`): its bootstrap now loads 5,163 lines of its own .pl natives (prolog/, lib/natives.pl)
   through `import_prolog_functions_from_file` with paths computed from `working_dir`, and something on that
   path does not work on CeTTa. Making 33cca343 run is CeTTa/PeTTaChainer porting work, not S0's (and nothing
   goes to Zar's CeTTa), so it was not diagnosed further.

So the profiles are of **PeTTaChainer b0e24f9b**, the revision CeTTa itself declares compatible, run exactly as
CeTTa's harness runs it. It is real PeTTaChainer code (the same chainer architecture as 33cca343, three months
older; 1,452 equations under 1,112 heads in its runtime .metta files plus the PeTTa library overlay, examples,
tests and benchmarks excluded), but the four PeTTaChainer functions fnspec compiled from 33cca343 (rs_* sites) are not all
present under the same names; the classification in section 4 is therefore made on b0e24f9b's own definitions,
with the fnspec compiler at this commit.

## 2. Instrumentation (upstream/cetta-s0/0005, CETTA_FNPROF)

A build flag, off by default: `make CETTA_FNPROF=1` adds `-DCETTA_FNPROF=1` and the build tag `.fnprof`; with the
default `CETTA_FNPROF=0` every hook is inside `#if CETTA_FNPROF` and compiles to nothing: no new fields in the
machine's goal, choice or machine structs, no calls. The off binary's .text is byte-identical to the
uninstrumented build's except 67 four-byte immediates of `mov edx, imm32` instructions, the source line numbers
passed to diagnostics, which the patch shifts (evidence/s0-2026-11-24/binaries.txt); it has no profiler symbols.
The report is written at exit to `$CETTA_FNPROF_OUT` (JSON). The profiler lives in PeTTa's search machine
(src/petta_search_machine.c), which CeTTa uses for PeTTa code with CETTA_PETTA_SEARCH_MACHINE=1 (CeTTa's
compatibility harness sets it), plus one hook in eval.c (space resolution) and one in space.c (published
mutations). Only the thread that first reaches a hook is profiled (other threads' events are counted and
dropped; there were none).

- **Frames.** Every machine goal and choice point carries the profiler frame of the activation that scheduled
  it. Three things open a frame: an equation candidate (parent: the frame of the call's choice point; goals the
  body schedules inherit it, and so do alternatives restored on backtracking), a call CeTTa runs through its
  prepared pure-call path outside the goal machine (the frame spans the host callback; same function row), and
  a space query (`match`, entered at petta_machine_start_ready_match_mode): a **match site** row
  `match@F S (H ...)`, the queries of function F on space S with pattern head H, so the matching and its
  template's own work are timed apart from F's other code. Destroying a machine returns to the frame that
  started it. The frames form the dynamic call tree of the run, including the branches search abandoned.
- **Self time** is wall time between two frame switches, charged to the frame that was current. Builtins,
  Prolog calls, the matcher behind a prepared call and child machines (collapse, once) are charged to the frame
  running them. Time outside any frame (loading, top-level evaluation) is `<outside-equations>`.
- **Inclusive time** of a function: the sum, over its frames with no ancestor frame of the same function, of
  the frame's self time and all its descendants' (recursion counted once).
- **Counts.** calls: first alternatives of equation choice points plus prepared calls that were taken (a call
  whose first alternative CeTTa selects before the choice point exists shows calls 0); candidates: equations
  tried (for a match site: rows handed back through a match choice point); bodies: frames opened; results:
  answers that left the frame (a switch from a frame to one of its ancestors at a goal pop, an emitted answer, a
  taken prepared call).
- **Space dependency vector.** Every space reference the machine resolves (match, add-atom, remove-atom, count,
  get-atoms: eval.c petta_eval_machine_resolve_space) is an access by the current frame's function or site to
  that space; every published revision of a space (space.c space_publish_mutation) is a write, with CeTTa's own
  classification: append-only or prefix-rewriting, and whether the equation projection changed (a definition
  changed). Per (function, space): accesses, writes (append/rewrite/equation), and how many accesses found a
  different space revision than that function's previous access.
- **Effect summary** per function, self and inclusive (over the frame tree): pure (no access), reads, writes.
- **Temporal stability.** Per function: how often &self's equation revision and data revision changed between
  two consecutive calls (what a per-&self-revision guard like fnspec's would see). Per space: mutations,
  inside functions, after its first access, append-only vs rewritten, equation mutations.
- **Guard checks and overhead for compiled sites.** No compiled site runs inside these workloads: S0 adds no
  compiled code (stop-line) and the fnspec sites run only through cettapack's host, so there is nothing to count;
  section 6 says what S1 has to add.

Limits of the attribution. Calls CeTTa answers from tables or memo without running an equation have no frame
(their cost is the caller's). Inside a prepared call there are no match-site frames (that path has its own
evaluator), so its matches are part of the prepared function's self time. Frames are kept to the end of the run
(24 bytes each; 1.5 M frames on the largest run).

## 3. Workloads

All three run real PeTTaChainer code (b0e24f9b, unmodified) on CeTTa with synthetic data only; scripts and
generators are committed (bench/s0), the generated programs' sha256 are in runs/<variant>/env-<variant>.txt.

| workload | what | files | queries | checks |
|---|---|---|---|---|
| w1-examples | PeTTaChainer's own examples: the three of CeTTa's compatibility manifest (deductionrevision, flyingraven, raveninduction) and the other eight of b0e24f9's examples/supported.txt that run on CeTTa (base_rate_acceptance_game_libpln needs lib_pln and stops at load) | 11 | their !(test ...) queries | every test passes (0 "❌" in every run, every build) |
| w2-wmtm | wmtm-style reasoning run (gen_workloads.py w2-wmtm, seed 20261124): 24 classes from wmtm's synthetic fixture vocabulary in an Inheritance taxonomy, 40 instances, 10 per-note Implication rules (CTV), goals: 10 instances against a taxonomy ancestor 3-4 inheritance steps away, and 4 goals labelled "neg" (an instance against a class that is neither a taxonomy ancestor nor reachable through the note rules; not a must-fail control, see 3.1), query budget 100 steps | 1 | 14 | the answers are S0's recorded stdout, byte-identical in every run (the check that holds); at 100 steps **5 of 10** positive goals derive an answer (confidence 0.002-0.02) and **1 of the 4** "neg" goals derives a weak answer by inversion (STV 0.68, confidence 0.03); corrected in S1, see 3.1 |
| w3-benchgen | larger KB: PeTTaChainer's benchgen statements (benchmarks/benchgen_metta.metta) ported with seeded truth values (gen_workloads.py w3-benchgen, seed 20261124, depth 3, width 3, 3 paths, noise factor 10, backward ratio 50%): 3,957 compileadd statements (120 true ones: 81 base facts and 39 main rules; 1,200 noise rules, 600 backward and 600 forward; benchgen's meta and tracking statements for all of them), 3 BenchTarget queries at 1,000 steps | 1 | 3 | all 3 targets proved |

### 3.1 Correction (S1, review finding 1): what w2's goal check shows

This section replaced, in S1 (t-2026-11-24-cettapack-s1-hot-sites), the w2 check S0 stated: "7 of 10 positive
goals derived, the 4 negative controls empty". Neither half holds, and the "negative controls" are not controls.
Recomputed from S0's own nine w2 runs (base/off/on x 3, all byte-identical) by bench/s1/check_w2_goals.py
(evidence/s1-2026-11-24/finding1/s0-runs-goals.md), and rerun on S1's base build at the S0 setting and at a
1,000-step budget (finding1/steps100, finding1/steps1000; programs and their sha256 in finding1/):

| goals | at 100 steps (S0's setting) | at 1,000 steps |
|---|---|---|
| 10 positive (instance against a taxonomy ancestor 3-4 inheritance steps away) | **5 derive an answer** (strength 1.0, confidence 0.002-0.02); 5 answer () | 10 derive (confidence 0.014-0.90) |
| 4 "neg" (a class that is neither a taxonomy ancestor of the instance nor reachable through the note rules) | **1 derives** (e_savings c_panel, by inversion: STV 0.68, confidence 0.03); 3 answer () | 4 derive (strength 0.63-0.69, confidence 0.03-0.13) |

- The 5 positives empty at 100 steps are budget-limited, not underivable: with 1,000 steps all 10 derive
  (718 s and 966 MB for the file, against 27 s and 112 MB at 100 steps).
- The "neg" goals share the KB's vocabulary (the same instances and classes as the positives), so they are not
  trivially empty, but under PeTTaChainer's PLN rules they are not must-fail either: inversion (abduction from the
  class side) gives weak evidence for any instance-class pair the KB connects, and given budget all four get an
  answer. The generator (gen_workloads.py) chooses them only against the taxonomy, so the label "negative control"
  claimed more than the construction shows. What separates them from the positives at 1,000 steps is strength
  (0.63-0.69 against 0.68-1.0) and confidence, not derivability, and the ranges overlap.
- The workload itself is unchanged (S1 measures on the same program, so S0's profile and estimates stay its
  baseline). What w2 checks per run is what S0's acceptance actually rested on: the answers are byte-identical to
  S0's recorded stdout (sha256 bbda4ea3...) in every run of every build. The goal counts above are a description of
  that fixed answer set, not a pass/fail control.

## 4. Results

Full tables (every workload: answers and overhead per file, the ranked rows with calls, candidates, results,
self and inclusive time, effect, space vector, &self revision changes, class; coverage and candidate extensions;
spaces; hot rows and the spaces they read): evidence/s0-2026-11-24/summary.md, generated by
bench/s0/summarize_s0.py from the raw runs and classification.json (`--check` recomputes byte for byte).

### 4.1 Answers and overhead (acceptance 2)

Every file's stdout is byte-identical across all runs of the three builds (base = a9b3a811 + 0001-0004 with
Prolog interop; off = + 0005 with CETTA_FNPROF=0; on = + 0005 with CETTA_FNPROF=1): **yes**, and no test fails.

| workload | base user s (sum of per-file medians) | off/base, paired by run: median (min-max) | on/base, paired by run: median (min-max) |
|---|---|---|---|
| w1-examples | 45.7 | 1.057 (0.972-1.086) | 1.081 (1.036-1.094) |
| w2-wmtm | 23.1 | 1.122 (1.013-1.139) | 1.043 (0.990-1.226) |
| w3-benchgen | 70.0 | 0.964 (0.898-1.048) | 1.100 (0.972-1.169) |

The off build's machine code is the base build's (section 2), so its ratio measures the host, not the flag: the
host ran other services throughout (load average 4-5 on 8 cores), and single runs moved by up to a third. The on
build's cost is the frame bookkeeping, one clock read per frame switch and the report.

### 4.2 Where the time goes

**w1-examples** (49.0 s profiled): advance-frontier-waiter-heap 13.2% self (60% incl., outside); frontier-remaining-score 6.8% self (8% incl., outside); commit-proof-candidate-heap 6.7% self (56% incl., outside); spawn-frontier-heap 5.5% self (51% incl., outside); spawn-frontier-goal-heap 5.3% self (31% incl., outside); proof-output-key-by-id 3.6% self (4% incl., outside).

**w2-wmtm** (24.3 s profiled): spawn-frontier-goal-heap 7.3% self (26% incl., outside); advance-frontier-waiter-heap 7.2% self (36% incl., outside); match@rule-goal-full-view ccls_index 5.9% self (6% incl., extension); frontier-remaining-score 4.9% self (6% incl., outside); rule-expected-score 4.7% self (24% incl., outside); goal-merged-proof-ids 3.9% self (6% incl., outside).

**w3-benchgen** (79.6 s profiled): externalize-compiled-addition 7.3% self (9% incl., outside); match@forward-register-base-rate! &forward_base_rate_interest 5.3% self (5% incl., extension); add-to-kb 4.3% self (23% incl., outside); advance-frontier-waiter-heap 3.9% self (23% incl., outside); internalize-runtime-addition 3.7% self (4% incl., outside); store-rule-expected-score! 2.9% self (4% incl., outside).

### 4.3 Coverage by compiled shapes, and the extensions (acceptance 3)

| workload | compiled now | + all named extensions | outside the method | outside any frame | rows touching a space (upper bound) | space accesses made in match-site rows |
|---|---|---|---|---|---|---|
| w1-examples | 0.21% | 2.49% | 93.3% | 4.2% | 69.9% | 7.9% |
| w2-wmtm | 0.31% | 8.51% | 90.1% | 1.4% | 69.8% | 12.8% |
| w3-benchgen | 0.17% | 14.27% | 77.3% | 8.4% | 60.9% | 16.1% |

Candidate extensions, by self time gained (cumulative coverage in summary.md):

| extension | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| named spaces written as bare symbols + conditions outside the fragment (a unification (= ...) or a call in an if condition) | 0.85% | 5.91% | 1.30% |
| comparisons against compound terms holding variables (=alpha on quoted tuples) | 0.03% | 0.61% | 5.27% |
| lift the match subexpression into a site | 1.04% | 0.77% | 2.46% |
| patterns in the function head (destructure before the match) | 0.09% | 0.11% | 3.62% |
| templates that call other functions or bind with let | 0.25% | 0.71% | 1.42% |
| named spaces written as bare symbols + lift the match subexpression into a site | 0.01% | 0.07% | 0.02% |
| multi-equation functions (one compiled dispatch over several equations) | 0.00% | 0.00% | 0.00% |
| nested match under collapse (row order of the flattened conjunction) | 0.00% | 0.00% | 0.00% |

## 5. What S2 and S3 will face

Section "Spaces" and "Hot rows and the spaces they read" of summary.md, per workload. Three kinds of change:

- **Definitions (&self).** PeTTaChainer compiles every `compileadd` into new equations, so &self's equation
  revision moves during a run; but it moves between top-level statements, not inside a query. A hot function sees
  a different &self equation revision between two consecutive calls tens of times per workload (52-100 times for
  the top w1 functions over 1,900-30,000 calls; about once per query in w2; 1-10 times in w3), always at the
  boundary between a KB update and the next query. A per-&self-revision guard (fnspec's, S2's dependency vector)
  would revalidate once per KB update, not per call; S2 has to make that revalidation cheap or batch it, since
  w3 interleaves ingestion with queries.
- **Search state: rewritten, not append-only.** The rows that take the time read PeTTaChainer's per-query working
  spaces (&goal, &proof, &goal_live_cache, &proof_live_cache, &goal_waiter, &goal_lookup, &proof_static_live,
  &aggregate). They are mutated throughout every query, by the hot rows themselves, and 30-50% of their
  publications rewrite the prefix (removals): w1 &goal 5,327 append-only / 4,590 rewritten, &proof 1,745 / 826;
  w3 &goal 11,527 / 10,502. Between two reads by the same row the revision has usually moved (13-100% of reads,
  depending on row and space). Guards on these spaces fail constantly; S3's patchable append-only spaces do not
  cover them, and a compiled site over them needs either no cached index (fnspec's plain guarded re-query) or an
  index maintained under deletion.
- **KB-side indexes: append-only while the KB is only ingested.** &forward_frontier,
  &forward_canonical_evidence and &forward_base_rate_interest are append-only in every workload. prem_index,
  ccls_index, rules and rule_expected are append-only in w3 (pure ingestion: 2,478-2,634 publications each, 0
  rewritten) but 15-35% rewritten in w1 and w2, where queries add temporary rules and erase them afterwards
  (PeTTaChainer's temporary query backend). In w3 half of the reads of these spaces by add-to-kb and the
  forward-chainer rows see a new revision, because reads and appends alternate during ingestion. These are S3's
  case: append-only, so a compiled site's index can be patched instead of rebuilt, as long as the query-time
  temporary rules are kept out of it (or the patching handles their removal). CeTTa marks many of these publications (and those of &goal_waiter, &goal_lookup,
  &proof_static_live) "equation or opaque" (it cannot show they leave the equation projection alone), so a guard
  keyed on a space's equation revision would also move on them.
- **Effectively constant:** &logic_config (11 append-only mutations per program, all at load).
- One unnamed space (space#N in the tables: never resolved by name) takes 27,000+ append-only publications at
  load, outside any frame; by its shape it is the import space of the loaded PeTTaChainer modules.

## 6. Guards and S1

- No compiled site runs in these workloads (S0 compiles nothing; fnspec sites run only in cettapack's host),
  so guard checks and guard overhead per compiled site are not measured here. The report has room for them: S1
  should register each compiled site with the profiler (a frame per specialised call, with its guard checks,
  refusals and residual calls counted on that row) when it puts compiled code inside CeTTa's runs, so the same
  tables show the compiled row's time next to the generic one.
- The two biggest single extension targets are match sites that are hot by themselves: w2's
  rule-goal-full-view on ccls_index (about 6% of w2; a bare-symbol named space, and a unification test (= ...)
  in the condition), and w3's forward-register-base-rate! on &forward_base_rate_interest (about 5% of w3;
  =alpha against quoted tuples holding variables; 2,400 calls at about 1.7 ms each). "Lift the match
  subexpression into a site" (compiling the collapse/once/existence + match part of a function that does more
  around it) is the most frequent extension (8-162 rows) but each row is small.
- Most self time is in rows with no space query at all (frontier/agenda management, proof-store bookkeeping,
  ingestion: advance-frontier-waiter-heap, spawn-frontier-heap, frontier-remaining-score,
  commit-proof-candidate-heap, externalize/internalize-runtime-addition). fnspec's method does not apply to
  them; whatever speeds them up (a native priority queue or agenda, cheaper proof-store updates) is a different
  method.

## 7. Limits

- **Pin.** b0e24f9b, not 33cca343 (section 1.1). The fnspec PeTTaChainer sites (33cca343) are not all present
  under the same names; the classification uses b0e24f9b's own definitions.
- **Attribution.** Calls CeTTa runs inline (specialised or inlined helpers such as *_Spec_[...]), table or memo
  answers, and the space queries inside prepared calls have no frames of their own; their time is in the
  enclosing row. Only 8-16% of space accesses happen in match-site rows (the rest are add/remove, counts, and
  queries in prepared or inlined code), so "outside" rows hold some query work that compiling queries could
  reach: the coverage figures for compiled shapes and extensions are lower bounds, and the "rows touching a
  space" column (61-71%) is the (loose) upper bound. Narrowing that gap needs timing inside CeTTa's matcher
  (every backend path), which S0 did not add.
- **Classification** is by fnspecc's first refusal (plus one retry for bare-symbol space names); an extension
  named here may meet a second refusal when it is built.
- **Counts.** calls counts first alternatives of equation choice points and taken prepared calls; a call whose
  first alternative CeTTa selects before the choice point exists shows calls 0 (candidates is then the count).
  Results count answers leaving a frame, including answers of branches the search later abandons.
- **Timing.** Shared host (other services kept the load average at 4-5 on 8 cores): per-run ratios move by up
  to a third; three interleaved runs per build. Self times come from the on build, which is itself slower.
- **w2** reproduces the shape of wmtm's KB and goals on wmtm's synthetic fixture vocabulary; it is not wmtm's
  own code path (wmtm drives PeTTa through Python).
  Its goals are not a derivability check: at 100 steps 5 of 10 positives and 1 of 4 "neg" goals answer, and the
  "neg" goals are not must-fail under PLN (section 3.1, corrected in S1).
## 8. Reproduce

```
# SWI-Prolog >= 9.3 foreign API, user-local (this host: 10.0.2 from source, see 1.1)
#   cmake -G Ninja -DCMAKE_INSTALL_PREFIX=<root> -DUSE_GMP=OFF -DSWIPL_PACKAGES_X=OFF -DSWIPL_PACKAGES_JAVA=OFF \
#         -DSWIPL_PACKAGES_ODBC=OFF -DSWIPL_PACKAGES_PYTHON=OFF -DSWIPL_PACKAGES_TERM=OFF -DINSTALL_DOCUMENTATION=OFF ..
export SWIPL_PKG_CONFIG_PATH=<root>/share/pkgconfig GMP_PREFIX=<libgmp-dev root>/usr
bench/s0/build_s0_cetta.sh base      # a9b3a811 + 0001-0004, ENABLE_LIB_PROLOG=1
bench/s0/build_s0_cetta.sh off       # + upstream/cetta-s0/0005, CETTA_FNPROF=0 (default)
bench/s0/build_s0_cetta.sh on        # + upstream/cetta-s0/0005, CETTA_FNPROF=1
RUNS=3 bench/s0/run_all.sh           # evidence/s0-2026-11-24/runs/{base,off,on}/<workload>/...
python3 bench/s0/classify_heads.py --image build/s0/image-on \
    --profiles 'evidence/s0-2026-11-24/runs/on/*/*.fnprof.json' --out evidence/s0-2026-11-24/classification.json
python3 bench/s0/summarize_s0.py evidence/s0-2026-11-24          # writes summary.md, ranking.json
python3 bench/s0/summarize_s0.py evidence/s0-2026-11-24 --check  # recompute: byte-identical or exit 1
```

The image (PeTTaChainer b0e24f9b subtree + PeTTa e038e4db lib overlay + the generated programs) is rebuilt by
run_workloads.sh from the two git checkouts (CHAINER_REPO, PETTA_REPO; defaults under scratch/survey) and the
wmtm fixture (WMTM_FIXTURE, sha256 checked).

## 9. Files

- upstream/cetta-s0/0005-fnprof-opt-in-PeTTa-function-profiler-CETTA_FNPROF-1.patch: the profiler (CeTTa
  src/cetta_fnprof.h new; src/petta_search_machine.c, src/eval.c, src/space.c, Makefile). Kept out of
  upstream/cetta so the pinned series (and its digest in every pack's profile, make test) is unchanged.
- bench/s0/build_s0_cetta.sh, run_workloads.sh, run_all.sh, gen_workloads.py, classify_heads.py, summarize_s0.py.
- evidence/s0-2026-11-24/: runs/ (raw stdout/stderr/time/exit and fnprof.json per run), env-*.txt (inside
  runs/<variant>/), classification.json, summary.md, ranking.json, compat/ (CeTTa's PeTTaChainer harness on the
  base build; the 33cca343 attempt), binaries.txt (build heads, patch digest, symbol and .text checks),
  make-test.log.
