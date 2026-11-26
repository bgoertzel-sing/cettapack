# S1: compiling the hot PeTTaChainer sites S0 found, and sizing the search-episode lever

Task t-2026-11-24-cettapack-s1-hot-sites (goal 2026-11-24-cettapack-roadmap, stage S1, re-planned from S0;
order projects/cettapack/orders, protocosmopopos). Branch hive/t-2026-11-24-cettapack-s1-hot-sites.

## Summary

**Question.** How much real PeTTaChainer time can guarded specialisation take: first by compiling the hot match sites
S0 ranked, then by sizing what Ben's Milestone 5 (a native search episode: choice frames, trail rollback, scratch
arenas, streamed answers) could win on the frontier/agenda and proof-store code that dominates.

**Answer.** Very little, and M5 as named would not change that. One hot site could be compiled: w3's
forward-register-base-rate! interest match (S0: 5.3% of w3). It now runs as compiled code inside CeTTa's own PeTTa
machine, through a new opt-in hook (patch file 0007). The guards are fnspec's: the definition is checked per &self
revision, and builtins, inputs, values and row order are checked per call. Its answers are CeTTa's in every one of
195 end-to-end file-runs and in 8 bag-test programs, including forced-invalid and mid-run redefinition cases, which
take the residual. The profiler shows the compiled site at **0.46x** of CeTTa's time for its own row on w3: 3.7 s
down to 1.7 s, about 2.7% of w3. End to end that is below the shared host's noise: the packs on/off paired user-time
ratio is 0.979 (0.843-1.082) on w3, 0.987 (0.906-1.053) on w2 and 1.040 (0.991-1.168) on w1, with peak memory within
2.4%. Coverage by compiled code inside real runs goes from 0 to 0.03% / 0.61% / 5.27% of self time (w1/w2/w3), against
S0's estimate of 2.5% / 8.5% / 14.3% for all extensions. It stops there. S0's other hot site (w2's ccls_index match,
5.9%) is a relation cursor plus a binding unification, which the method cannot express. Samples bound what compiling
every other site's condition and template could take at under 1% of each workload, outside that site.

**Search episode (M5).** The backward search episode takes 92% / 88% / 47% of w1/w2/w3 CPU time. Choice points, trail
and backtracking, M5's central mechanism, are only **1.0-1.4%** of it: PeTTaChainer searches with an explicit agenda
(Prolog heaps and spaces) driven by deterministic functions, not with MeTTa nondeterminism. The cost is per-call
interpretation: equation selection and call preparation 18-22%, matching 20-24%, goal dispatch 15%, allocation 14%,
copying 10%, hashing 9-10%. Estimates, under stated assumptions: M5 as named would save 2-11% of run time (1.02-1.13x).
The full native episode of Ben's section 11 would save at most 12-64% (1.13-2.8x), but only by also compiling the
chainer's search functions themselves (fixed rule dispatch, registers; 25-26 functions hold 90% of the search
code's self time). That amounts to a guarded compiler for
deterministic MeTTa functions, not a search machine. **Recommendation: M5 as defined should not be the next stage.**

**Open, needs a decision.** If speeding up PeTTaChainer is the goal, two directions remain:
- a stage that compiles the agenda's deterministic functions (let/case/if chains, space updates, Prolog heap calls),
  guarded per definition; or
- per-call costs inside CeTTa itself. One example: prepared_pure_program_cache_has_decline_head walks a list on every
  call and alone is 2.6-4.9% of search time. That is a CeTTa change, which is Zar's call.

Nothing was sent to Zar's CeTTa; the patches are files.

## 1. Inputs and pins

| what | pin | note |
|---|---|---|
| cettapack | f8126a1 = S0 tip (hive/t-2026-11-24-cettapack-s0-profile), directly on main efcfdd3 | "main with S0 merged": S0 was not merged into main when S1 started; f8126a1 is that fast-forward |
| CeTTa | a9b3a811 + upstream/cetta/0001-0004 + upstream/cetta-s0/0005 (files, unchanged) | as S0 |
| S1 patches (files) | upstream/cetta-s1/0006 (sampler), 0007 (compiled match-site hook) | nothing goes to Zar's CeTTa |
| SWI-Prolog | 10.0.2, user-local (scratch/deps/swipl10) | as S0 |
| PeTTaChainer | b0e24f9b (pettachainer/metta subtree) | as S0 |
| PeTTa | e038e4db, lib/lib_*.metta overlaid | as S0 |
| workloads | S0's w1-w3, unchanged programs (w2 sha256 ec4a904c..., w3 8fca85cd...) | bench/s0/gen_workloads.py, seed 20261124 |

Builds (bench/s1/build_s1_cetta.sh; sha256 and build heads in evidence/s1-2026-11-24/binaries.txt):
`base` (S0's base: 0001-0004, ENABLE_LIB_PROLOG=1, rebuilt in this worktree), `sites` (0001-0007,
CETTA_FNPROF=0, CETTA_S1_SITES=1, with src/cpk_s1_sites.c as the compiled sites), `prof` (0001-0006,
CETTA_FNPROF=1, -g: profiles and samples), `sitesprof` (sites + profiler).

Sanity gate (docs/EXPERIMENT_SANITY_GATE.md): the known-answer canary is S0's recorded stdout of every workload
file (S0 base run 1); every S1 run of every build writes a check line (bench/s1/run_workloads.sh: exit status,
stdout sha256, equal to S0's or not, failed tests), and every result below rests on runs whose line says
"same". Pilots before the long runs: evidence/s1-2026-11-24/{prof-pilot,pilot-sites,pilot-sites2,bagtest-pilot}.

## 2. S0 review finding 1 (committed first, 4c8faf7)

Taken to be: S0's description of w2's check, "7 of 10 positive goals derived, the 4 negative controls empty", was
wrong, and the "negative controls" are not controls. Recomputed from S0's own nine w2 runs and rerun here
(bench/s1/check_w2_goals.py, evidence/s1-2026-11-24/finding1): at S0's 100-step budget **5 of 10** positives derive
(confidence 0.002-0.02) and **1 of the 4** "negative" goals derives a weak answer by inversion (STV 0.68, confidence
0.03); at 1,000 steps all 10 positives and all 4 "negatives" derive. The empty positives are budget-limited; the
"negatives" share the KB's vocabulary but are not must-fail under PLN. Relabelled in docs/S0_PROFILE.md (new section
3.1, table row, limits) and in projects/cettapack/RESULT-s0-profile.md; the workload was not changed, so S1 measures
on S0's exact program and S0's profile stays its baseline. The per-run check that holds is byte-identical answers.

## 3. Compiling the sites S0 ranked

### 3.1 How compiled code runs inside CeTTa's PeTTaChainer runs

fnspec's sites ran only in cettapack's own host; S0 found that none ran inside a real PeTTaChainer run. S1 adds the
missing piece as a patch file, **upstream/cetta-s1/0007** (opt-in build flag CETTA_S1_SITES=1; with the default 0
nothing is compiled or called): when PeTTa's search machine starts a match on a space
(petta_machine_start_ready_match_mode), after typed pattern elaboration and only if that left pattern and template
as written, it offers the match to `cetta_s1_sites_try`. A compiled site that recognises the match and whose guards
hold returns the template's values for the matching rows; the machine answers them as an ordinary outcome choice,
so once/collapse/continuations above the match are unchanged. Otherwise the site declines, having changed nothing,
and the machine runs the match as before (the residual). The sites are cettapack code, src/cpk_s1_sites.c, built into
CeTTa as src/cetta_s1_sites.c; the run-time switch is $CETTA_S1_SITES (0 = packs off, 1 = on, invalid = every guard
forced invalid); counters go to $CETTA_S1_SITES_STATS, a per-call trace to stderr with $CETTA_S1_SITES_TRACE.

Guards, fnspec's carried into the machine:

| guard | checked | failing -> |
|---|---|---|
| forced invalid (CETTA_S1_SITES=invalid) | every call | residual |
| definition: &self holds exactly one equation of the enclosing function, alpha-equal to the one compiled (PeTTaChainer b0e24f9b's text verbatim, tools/s1_sitegen.py -> src/cpk_s1_sites_text.h); no catch-all equation; no equation in &self for a builtin the template uses (=alpha, and, if, quote, empty, match, once, collapse) | per &self program revision (space_program_token: equation and declaration revisions) | residual until the revision moves |
| recognition: the match (space, pattern, template) is the compiled one up to renaming; row variables unbound | every call | not this site (generic path) |
| inputs: the values in the pattern the match does not bind are ground; no list carrier left in the query | every call | residual |
| values: every operand the template evaluates is already a value (fnspec's normal-form check; unbound variables allowed, as PeTTa leaves them) after observing PeTTa list carriers as PeTTa does (petta_semantics_materialize_value) | every row | the call declines before answering; residual |
| order: at most one distinct answer per call (the compiled query, space_query_conjunction, may enumerate rows in another order than the machine's match, and the site sits under once) | every call | residual |

### 3.2 Site 1, compiled: forward-register-base-rate!'s interest match (w3's 5.3%)

PeTTaChainer forward_chainer.metta:385, the match S0 ranked first in w3 (and 0.45% of w2):

    (collapse (once (match &forward_base_rate_interest
       (forward-base-rate-interest-row $trigger $stored-id $kbid $stored-pattern $stored-extractor $stored-init
                                       $fun $finish $tv-mode)
       (if (and (=alpha $stored-pattern $pattern)
                (=alpha (quote ($stored-extractor $stored-init)) (quote ($extractor $init))))
           $stored-id (empty)))))

S0's refusal: "comparisons against compound terms holding variables (=alpha on quoted tuples)". The compiled row
evaluates the two =alpha tests with CeTTa's own atom_alpha_eq (one variable bijection per comparison, as PeTTa's
=alpha; quote's contents unevaluated, other operands required to be values), and answers $stored-id. Checked
against PeTTa first (/tmp probes recorded in the bag tests): =alpha on quoted tuples of terms with variables,
evaluated arguments outside quote, bijection failures, and list carriers compared with flat expressions.

What the generic path spends there (samples, evidence/s1-2026-11-24/prof-pilot, w3): 55% of the row's samples
inside petta_machine_dispatch_solve (the machine evaluating if/and/=alpha/quote goals per row), binding application
and copying 15-17%, builtin-equation checks per call 7%, the matcher itself about 11%.

A first pilot (evidence/s1-2026-11-24/pilot-sites) specialised almost nothing (w3: 5 of 2,399 calls): the chainer's
patterns reach the match as PeTTa list carriers ((cons Inheritance (cons $_ (cons c ())))), which the value check
refused. The site now observes values as CeTTa does before checking and comparing them; bag test t7_carriers.

### 3.3 Site 2, not compiled: rule-goal-full-view on ccls_index (w2's 5.9%)

    (match ccls_index ($lookup-goal $rule-id $args)
       (if (= $lookup-goal $goal) (data $rule-id (contextualize-rule-args $goal $args) $goal $parent-gid) (empty)))

ccls_index is a bare-symbol "space": PeTTa's dynamic relation (kb-add-atom ... ccls_index ...), which CeTTa runs
through its open-equation relation cursor (petta_machine_try_ready_relation_invocation, oem_cursor_run), not a
space the matcher reads. Samples (prof-pilot, w2): of this row's 6.1% of w2, 4.5% is the row source (relation
invocation, specializer analysis of the call, equation choice, the cursor) and at most 1.6% is anything after it.
That remainder is not a row filter: (= $lookup-goal $goal) is a unification whose bindings flow into $goal and the
continuation (contextualize-rule-args, the returned data), and the template calls a function. fnspec's method keeps
the row source and compiles a binding-free condition over data templates; this site has neither, so it stays
CeTTa's. (Its S0 class came from the first refusal only, "bare-symbol space + condition outside the fragment"; S0's
limits section warned an extension might meet a second refusal: it meets three.)

### 3.4 The rest, in order of time covered: coverage stops growing

bench/s1/site_reach.py splits every match-site row's samples into the row source (matcher, relation cursor: kept by
the method) and per-row work (condition, template, hand-back, glue: the upper bound on what compiling them removes):

#### w1-examples (136967 samples)

| match site | S0 class | samples % | row source % | per-row work % (bound) |
|---|---|---|---|---|
| match@rule-goal-full-view ccls_index ((...) ...) | extension | 0.96 | 0.71 | 0.25 |
| match@ensure-goal-at-depth &goal_lookup ((...) ...) | extension | 0.61 | 0.39 | 0.22 |
| match@wake-proof-commit-heap &goal_waiter ((...) ...) | extension | 0.25 | 0.14 | 0.12 |
| all match-site rows | | 2.39 | 1.68 | 0.71 |
| rows S0 classed 'extension' | | 2.29 | | 0.70 |

#### w2-wmtm (65722 samples)

| match site | S0 class | samples % | row source % | per-row work % (bound) |
|---|---|---|---|---|
| match@rule-goal-full-view ccls_index ((...) ...) | extension | 6.05 | 4.42 | 1.63 |
| match@compile-specialization-fact-triggers &spec_template (specialization-template-row ...) | extension | 0.66 | 0.55 | 0.10 |
| match@ensure-goal-at-depth &goal_lookup ((...) ...) | extension | 0.52 | 0.27 | 0.24 |
| match@forward-register-base-rate! &forward_base_rate_interest (forward-base-rate-interest-row ...) | extension | 0.51 | 0.10 | 0.41 |
| match@store-computed-base-rate! &base_rate_cache (base-rate ...) | compiled | 0.23 | 0.01 | 0.22 |
| all match-site rows | | 8.44 | 5.67 | 2.78 |
| rows S0 classed 'extension' | | 8.14 | | 2.55 |

#### w3-benchgen (189797 samples)

| match site | S0 class | samples % | row source % | per-row work % (bound) |
|---|---|---|---|---|
| match@forward-register-base-rate! &forward_base_rate_interest (forward-base-rate-interest-row ...) | extension | 5.39 | 0.19 | 5.20 |
| match@add-to-kb &forward_frontier (forward-frontier-row ...) | extension | 1.97 | 1.96 | 0.00 |
| match@forward-output-row-for-fact &forward_canonical_evidence (forward-canonical-evidence-row ...) | extension | 1.77 | 1.77 | 0.00 |
| match@compile-specialization-fact-triggers &spec_template (specialization-template-row ...) | extension | 1.40 | 1.00 | 0.40 |
| match@rule-goal-full-view ccls_index ((...) ...) | extension | 1.40 | 1.03 | 0.37 |
| match@ensure-goal-at-depth &goal_lookup ((...) ...) | extension | 0.31 | 0.25 | 0.06 |
| all match-site rows | | 12.65 | 6.49 | 6.16 |
| rows S0 classed 'extension' | | 12.47 | | 6.11 |

After site 1, what the method could still take is at most 0.7% of w1, 2.2% of w2 (1.6 points of it the ccls_index
site above, which the method cannot express) and 0.9% of w3, spread over sites each at or below 0.4%: the next
ranked w3 sites (add-to-kb on &forward_frontier, 2.0%; forward-output-row-for-fact on &forward_canonical_evidence,
1.8%) are all row source (the matcher unifying candidates, petta_semantics_match_cons_constraint_mode), and
compile-specialization-fact-triggers is a relation cursor again. Compiling them could not move the end-to-end time
by more than the host's run-to-run noise, so, by the order's criterion, coverage has stopped growing and no further
site was compiled. (The per-row-work bound is loose: candidate unification on choice advance counts as per-row work
on some paths, e.g. 5.16% for site 1, part of which is the matcher.)

### 3.5 Bag tests (acceptance 2)

bench/s1/run_bagtests.sh: eight PeTTa programs (bench/s1/bagtest/t0-t7, each prefixed by common.inc, which imports
PeTTaChainer and calls forward-register-base-rate! as the chainer does), each run in its own process in three modes
on the sites build: off (CeTTa's answers), on, and forced invalid. Pass = on and invalid stdout byte-identical to off,
exit 0, and the site counters equal to the route each test's construction predicts (bench/s1/bagtest/expected.json,
checked by check_routes.py; in invalid mode every recognised call residual).

| test | off = on | off = invalid | routes (on) | invalid mode | verdict |
|---|---|---|---|---|---|
| t0_first_call | yes | yes | recognised 3, specialised 3, residual 0  | forced 3/3 | pass |
| t1_basic | yes | yes | recognised 11, specialised 11, residual 0  | forced 11/11 | pass |
| t2_values | yes | yes | recognised 6, specialised 5, residual 1 {"residual.inputs": 1} | forced 6/6 | pass |
| t3_order | yes | yes | recognised 2, specialised 1, residual 1 {"residual.order": 1} | forced 2/2 | pass |
| t4_redefine | yes | yes | recognised 9, specialised 5, residual 4 {"residual.invalid": 4} | forced 9/9 | pass |
| t5_replace | yes | yes | recognised 3, specialised 1, residual 2 {"residual.invalid": 2} | forced 3/3 | pass |
| t6_value | yes | yes | recognised 3, specialised 2, residual 1 {"residual.value": 1} | forced 3/3 | pass |
| t7_carriers | yes | yes | recognised 5, specialised 5, residual 0  | forced 5/5 | pass |

Covered: alpha-variants hitting a stored row and non-variants adding one, other extractors and inits (t1); a call with a
non-ground KB (inputs guard, t2); two stored rows passing for one call (order guard, t3); mid-run a second equation of
the function, then removed, and =alpha shadowed by an equation in &self, then unshadowed (t4); the function replaced
by a different equation holding the same match, then restored (t5); a stored pattern that becomes evaluable when its
head gets an equation (value guard, t6); patterns built as PeTTa list carriers, alpha-equal across carrier and flat
forms (t7). Found on the way and recorded: the first call of a run, before anything was added to
&forward_base_rate_interest, never reaches the site (the token is unbound and CeTTa runs that match as a plain-symbol
query before the hook; t0 checks the converse); a carrier with an open tail makes CeTTa itself stop with
"observation incomplete" in every mode, so it is not a bag test (the site declines it).

## 4. End to end, packs off and on (acceptance 3)

Arms (bench/s1/run_e2e.sh, RUNS=5, rounds interleaved with the arm order rotating; bench/s1/summarize_e2e.py,
`--check` recomputes): **base** = S0's base build; **off** = the S1 sites build with CETTA_S1_SITES=0 (packs off);
**on** = the same binary with CETTA_S1_SITES=1 (packs on). Full tables: evidence/s1-2026-11-24/e2e/summary.md.

### 4.1 Answers

| workload | arm | runs x files | exit 0 | stdout = S0 base | failed tests |
|---|---|---|---|---|---|
| w1-examples | base | 5 x 11 | 55/55 | 55/55 | 0 |
| w1-examples | off | 5 x 11 | 55/55 | 55/55 | 0 |
| w1-examples | on | 5 x 11 | 55/55 | 55/55 | 0 |
| w2-wmtm | base | 5 x 1 | 5/5 | 5/5 | 0 |
| w2-wmtm | off | 5 x 1 | 5/5 | 5/5 | 0 |
| w2-wmtm | on | 5 x 1 | 5/5 | 5/5 | 0 |
| w3-benchgen | base | 5 x 1 | 5/5 | 5/5 | 0 |
| w3-benchgen | off | 5 x 1 | 5/5 | 5/5 | 0 |
| w3-benchgen | on | 5 x 1 | 5/5 | 5/5 | 0 |

Every run of every arm exits 0 with S0's answers and no failed test: **yes**.

### 4.2 Time and peak memory (sum over the workload's files per run; median (min-max) over 5 runs)

| workload | arm | runs | user s | wall s | peak RSS MiB |
|---|---|---|---|---|---|
| w1-examples | base | 5 | 39.59 (36.51-49.79) | 40.19 (36.91-50.62) | 154 (153-154) |
| w1-examples | off | 5 | 42.93 (36.59-44.65) | 43.46 (37.00-45.24) | 154 (154-154) |
| w1-examples | on | 5 | 46.45 (37.38-50.16) | 47.01 (37.86-50.87) | 153 (152-153) |
| w2-wmtm | base | 5 | 22.03 (20.18-27.90) | 22.13 (20.25-28.76) | 111 (110-111) |
| w2-wmtm | off | 5 | 23.42 (19.32-26.68) | 23.54 (19.39-26.78) | 110 (110-111) |
| w2-wmtm | on | 5 | 24.17 (19.06-24.65) | 24.26 (19.15-24.77) | 113 (113-114) |
| w3-benchgen | base | 5 | 65.39 (58.46-69.43) | 65.88 (58.78-69.90) | 310 (310-311) |
| w3-benchgen | off | 5 | 66.23 (58.15-77.45) | 66.66 (58.51-78.00) | 311 (310-311) |
| w3-benchgen | on | 5 | 65.30 (56.47-71.67) | 65.71 (56.76-72.22) | 310 (310-311) |

Paired, run r of one arm against run r of another:

| workload | runs | on/off user | on/off wall | off/base user | on/base user | on/off peak RSS |
|---|---|---|---|---|---|---|
| w1-examples | 5 | 1.040 (0.991-1.168) | 1.039 (0.992-1.170) | 1.005 (0.862-1.123) | 1.034 (0.944-1.298) | 0.993 (0.990-0.998) |
| w2-wmtm | 5 | 0.987 (0.906-1.053) | 0.988 (0.906-1.052) | 0.957 (0.839-1.177) | 0.944 (0.884-1.220) | 1.024 (1.022-1.031) |
| w3-benchgen | 5 | 0.979 (0.843-1.082) | 0.979 (0.842-1.083) | 1.013 (0.928-1.121) | 0.978 (0.901-1.096) | 0.999 (0.995-1.002) |

The site in the on arm (identical counters in every run; answers = S0's result count for this row in every workload:
89, 200, 1,170; recognised = S0's calls minus each file's first call, section 3.5):

| workload | runs | recognised | specialised | residual (by reason) | rows read | answers | same in every run |
|---|---|---|---|---|---|---|---|
| w1-examples | 5 | 158 | 158 | none | 509 | 89 | yes |
| w2-wmtm | 5 | 263 | 263 | none | 9485 | 200 | yes |
| w3-benchgen | 5 | 2399 | 2399 | none | 323266 | 1170 | yes |

No end-to-end change is measurable on this host. The expected effect, from the profiler row below, is about 2.7% on
w3, 0.3% on w2 and +0.03% on w1. The run-to-run range is wider than that: single w1 files swing between 0.53x and
2.08x on/off in rounds 1-3 and sit at 0.84-1.17x in rounds 4-5, in both directions. The w3 median (0.979) matches the
expectation but does not establish it. off/base (1.005, 0.957, 1.013) shows that the hook, compiled in but switched
off, costs nothing measurable.

### 4.3 The compiled site's own row (profiler, sitesprof build, sites off vs on alternating, 3 runs)

bench/s1/run_sprof.sh, bench/s1/sprof_table.py (evidence/s1-2026-11-24/sprof/summary.md). The profiler's match-site
frame opens before the hook, so the row holds CeTTa's generic match (off) or the compiled site (on: guards, query,
compiled rows, hand-back):

| workload | runs | row self ms, off | row self ms, on | on/off (row) | parent incl. ms off | on | total ms off | total ms on | row share of total, off |
|---|---|---|---|---|---|---|---|---|---|
| w1-examples | 3 | 13.2 (11.6-14.0) | 26.1 (24.7-26.5) | 1.974 (1.897-2.123) | 41.3 (36.8-43.9) | 57.4 (52.1-60.0) | 40583 (39153-45297) | 47837 (39446-48181) | 0.03% (0.03%-0.03%) |
| w2-wmtm | 3 | 124.6 (123.8-134.5) | 66.3 (65.1-78.5) | 0.535 (0.484-0.630) | 158.4 (157.5-170.9) | 102.0 (99.8-120.5) | 21921 (21237-25546) | 22233 (22140-26528) | 0.59% (0.48%-0.61%) |
| w3-benchgen | 3 | 3663.7 (3298.2-3801.5) | 1672.9 (1543.3-1687.1) | 0.457 (0.444-0.468) | 4078.6 (3655.9-4241.2) | 2095.1 (1939.9-2107.8) | 72854 (66367-73461) | 70662 (64328-75483) | 4.99% (4.97%-5.22%) |

On w3 the compiled site does the row in 0.46x of CeTTa's time (2.0 s saved of 72 s: 2.7%); on w2 in 0.54x (0.06 s of
22 s). On w1 (158 calls over 11 processes) the row costs 13 ms more in all: the site parses its equation and checks
the definition once per process (about 1 ms). Why not more: the compiled site reads every row of the trigger group
(about 135 per call on w3, 323,266 in all, against the generic path stopping at the first passing row under once),
because the order guard needs them all (Limits).

### 4.4 Coverage, against S0's estimate

| workload | S0: compiled shapes (fnspec, not running in CeTTa runs) | S0: + all named extensions (estimate) | S1: rows now answered by compiled code inside CeTTa's run (S0 self share of the site's row; every recognised call specialised) | S1: bound on what the method could still take (per-row work of all other extension sites, 3.4) | S1: time saved, profiler row |
|---|---|---|---|---|---|
| w1-examples | 0.21% | 2.49% | 0.03% | 0.67% (ccls_index 0.23 of it) | -0.03% (one-time cost) |
| w2-wmtm | 0.31% | 8.51% | 0.61% | 2.2% (ccls_index 1.61 of it) | 0.26% |
| w3-benchgen | 0.17% | 14.27% | 5.27% | 0.9% | 2.7% |

S0's estimate counted the whole self time of every row an extension would touch, matcher included. What compiling a
site can take is its per-row work only, and the matcher stays. That is why S1's reachable coverage is a fraction of
S0's estimate: 1% / 7% / 37% of it is now compiled and running, and the remaining per-row bound is under 1% of each
workload (outside ccls_index).

## 5. Search-episode sizing (step 4, report only)

Profiled runs: bench/s1/run_workloads.sh prof, three runs per file (evidence/s1-2026-11-24/prof-pilot = run 1,
prof-r2 = runs 2-3; every stdout S0's). The sampler (0006) took one native backtrace per ms of process CPU time with the
profiler's current row; bench/s1/analyze_samples.py symbolises them and bench/s1/step4_search.py groups rows by the
PeTTaChainer module that defines them, pooling the three runs (the pilot alone gives the same shares within about 1
point). The search episode = rows of backward_chainer.metta and backward_proof_store.metta (the frontier/agenda heaps,
waiters, proof candidates, proof store) and everything they call. Tables: evidence/s1-2026-11-24/step4/step4.md;
recompute: bench/s1/recompute_s1.sh.

### 5.1 Where the time is, by module (self, profiler's own samples removed)

| module | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| backward_chainer.metta | 66.1% | 64.4% | 33.0% |
| petta_chainer.metta | 2.0% | 4.6% | 18.2% |
| backward_proof_store.metta | 14.0% | 10.4% | 4.6% |
| chainer_utils.metta | 8.8% | 10.6% | 7.9% |
| forward_chainer.metta | 0.5% | 1.3% | 11.8% |
| <outside any equation> | 3.2% | 1.7% | 9.2% |
| compile.metta | 2.1% | 2.8% | 9.3% |
| compiled_query_runtime.metta | 0.8% | 0.9% | 3.2% |
| <other> | 1.1% | 1.6% | 1.6% |
| lib/lib_roman.metta | 0.9% | 1.1% | 0.4% |
| tv_formulas.metta | 0.6% | 0.4% | 0.7% |
| logic_config.metta | 0.0% | 0.0% | 0.0% |
| chainer_runtime_core.metta | 0.0% | 0.0% | 0.0% |
| lib/lib_import.metta | 0.0% | 0.0% | 0.0% |
| fold_inverse.metta | 0.0% | 0.0% | 0.0% |
| query_runtime.metta | 0.0% | 0.0% | 0.0% |
| lib/lib_spaces.metta | 0.0% | 0.0% | 0.0% |
| (profiler's own samples, removed) | 6.5% of all | 3.5% of all | 9.2% of all |
| samples (without profiler) | 137008 | 65731 | 189825 |

| | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| search rows, self | 80.1% | 74.8% | 37.6% |
| search episode, inclusive (search rows and all they call) | 92.1% | 88.4% | 47.0% |

### 5.2 What the episode spends its time on (inclusive, by native category)

| category | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| prolog (SWI-Prolog and the foreign boundary) | 4.4% | 6.1% | 7.4% |
| allocation and freeing | 13.5% | 13.6% | 14.9% |
| choice points, trail and backtracking | 1.4% | 1.2% | 1.0% |
| equation selection and call preparation | 22.0% | 17.7% | 19.4% |
| copying and instantiation | 10.2% | 10.5% | 10.3% |
| matching, unification and fact iteration | 22.7% | 23.7% | 20.3% |
| space mutation (add/remove atoms) | 0.7% | 0.9% | 1.0% |
| hashing, interning and term universe | 8.7% | 9.8% | 9.8% |
| grounded ops and arithmetic | 1.3% | 1.3% | 1.5% |
| goal dispatch and evaluation | 15.1% | 15.3% | 14.5% |
| other machine | 0.0% | 0.0% | 0.0% |

Inside the search rows' own code (self) the split is the same within 2 points (step4.md). Top native functions in the
search rows:

- **w1-examples**: prepared_pure_program_cache_has_decline_head 5.4%, __default_morecore@libc.so.6 4.6%, bindings_apply_fold_impl 1.9%, malloc@libc.so.6 1.8%, __nss_database_lookup@libc.so.6 1.7%, timer_settime@libc.so.6 1.5%, free@libc.so.6 1.1%, petta_eval_open_foreign_reads_current 1.1%, atom_has_vars 1.1%, bindings_lookup_value_id 1.0%, petta_machine_dispatch_solve 0.9%, petta_eval_open_program_lookup_locked 0.9%, atom_summary_add_ids 0.8%, petta_table_safety_scan_relation 0.7%, petta_eval_machine_builtin_equations 0.7%
- **w2-wmtm**: __default_morecore@libc.so.6 4.7%, prepared_pure_program_cache_has_decline_head 2.8%, malloc@libc.so.6 1.9%, bindings_apply_fold_impl 1.8%, __nss_database_lookup@libc.so.6 1.8%, timer_settime@libc.so.6 1.6%, free@libc.so.6 1.1%, bindings_lookup_value_id 1.1%, atom_has_vars 1.0%, atom_summary_add_ids 0.9%, petta_machine_dispatch_solve 0.9%, petta_eval_open_foreign_reads_current 0.8%, petta_eval_machine_builtin_equations 0.8%, pl_current_predicate1_va@libswipl.so.10.0.2 0.7%, petta_eval_open_program_lookup_locked 0.7%
- **w3-benchgen**: __default_morecore@libc.so.6 5.7%, prepared_pure_program_cache_has_decline_head 2.6%, malloc@libc.so.6 1.9%, timer_settime@libc.so.6 1.8%, __nss_database_lookup@libc.so.6 1.7%, bindings_apply_fold_impl 1.5%, free@libc.so.6 1.4%, pl_current_predicate1_va@libswipl.so.10.0.2 1.3%, petta_machine_dispatch_solve 0.9%, atom_has_vars 0.8%, atom_deep_copy_memo_lookup 0.8%, atom_summary_add_ids 0.8%, bindings_lookup_value_id 0.8%, petta_eval_open_foreign_reads_current 0.8%, petta_eval_machine_builtin_equations 0.7%

(libc's internal functions resolve to the nearest exported symbol, e.g. "__default_morecore" and "timer_settime" stand
for malloc/free internals and memory moves; their samples are categorised by the first named caller.)

### 5.3 Is it a search the machine backtracks through? (profiler counts, summed over files and the three runs; "functions.rows" counts a function once per file and run)

| | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| functions.rows | 5,286 | 501 | 459 |
| functions.calls | 663,351 | 290,988 | 392,835 |
| functions.candidates | 510,480 | 263,736 | 329,283 |
| functions.extra_candidates | 88,281 | 47,790 | 56,832 |
| functions.with_more_candidates_than_calls | 687 | 66 | 51 |
| functions.frames | 2,827,791 | 1,079,406 | 1,377,747 |
| functions.results | 752,316 | 346,002 | 451,374 |
| match sites.calls | 41,430 | 31,977 | 50,070 |
| match sites.candidates | 56,634 | 48,372 | 61,092 |
| match sites.results | 37,614 | 26,598 | 29,076 |

Search functions take one equation per call in most calls. Alternatives tried (extra candidates) are 13-16% of calls
and come from multi-equation functions dispatching on argument shape, not deep backtracking. The best-first agenda is
explicit: heaps in Prolog (search-agenda-heap, 42% Prolog), waiters and proof candidates in spaces rewritten
throughout every query (S0 section 5). So the parts the order names come out as:
- **allocation** 14-15% of the episode;
- **copying** (binding application, deep copies) 10%;
- **re-matching** (matching, unification and fact iteration: the same frontier spaces queried again and again) 20-24%;
- **choice handling** 1.0-1.4%;
- the rest: equation selection and call preparation 18-22%, goal dispatch 15%, hashing and interning 10%, Prolog 4-7%.

### 5.4 What a native search episode would win (estimate, bench/s1/m5_estimate.py)

Measured inputs: the episode share and composition above. Assumed: the fraction of each category each mechanism
removes:

| category | mechanism | removed, named M5 | removed, full native episode |
|---|---|---|---|
| choice points, trail and backtracking | choice frames + trail/watermark rollback | 50-100% | 50-100% |
| allocation and freeing | scratch arena mark/reset | 30-80% | 30-80% |
| copying and instantiation | registers for bound variables (needs compiled functions) | 0-0% | 30-80% |
| equation selection and call preparation | fixed rule dispatch (needs compiled functions) | 0-0% | 50-90% |
| goal dispatch and evaluation | compiled control (needs compiled functions) | 0-0% | 50-90% |
| matching, unification and fact iteration | PathMap fact iterators + exact rematch | 0-0% | 0-50% |
| hashing, interning and term universe | fewer fresh terms (follows from the above) | 0-0% | 0-50% |
| prolog (SWI-Prolog and the foreign boundary) | not M5 (the chainer's Prolog heaps; a native heap is another step) | 0-0% | 0-0% |
| space mutation (add/remove atoms) | not removed (the search state is in spaces) | 0-0% | 0-0% |
| grounded ops and arithmetic | not removed (TV formulas; M6's unboxed kernels) | 0-0% | 0-0% |
| other machine | - | 0-0% | 0-0% |

| workload | episode share (measured) | named M5: saved of run | speedup | full episode: saved of run | speedup |
|---|---|---|---|---|---|
| w1-examples | 92.1% | 4.4-11.2% | 1.05-1.13x | 24.3-63.9% | 1.32-2.77x |
| w2-wmtm | 88.4% | 4.1-10.7% | 1.04-1.12x | 21.5-59.1% | 1.27-2.45x |
| w3-benchgen | 47.0% | 2.3-6.1% | 1.02-1.06x | 11.7-31.3% | 1.13-1.46x |

- **Reachable share.** The episode itself: 92% of w1, 88% of w2, 47% of w3 (w3's other half is KB ingestion and
  forward chaining, outside any search episode).
- **M5 as named.** Choice frames, trail/watermark rollback, scratch arenas and streamed answers, applied to the episode
  CeTTa runs now, reach only the choice/trail (1.0-1.4%) and allocation (14-15%) shares: **2-11% of run time, 1.02-1.13x**.
  Its defining mechanism, choice frames with trail rollback, addresses about 1%.
- **Full native episode.** Ben's section 11 adds registers for bound variables, fixed rule dispatch and PathMap fact
  iterators. That reaches equation selection, dispatch, copying and part of matching: **at most 12-64%, 1.13-2.8x**.
  The upper end assumes the compiled code costs nothing.
- **What it would take.** Compiling PeTTaChainer's search functions themselves: 72-96 distinct search functions
  are called per workload (of 308 the two modules define), and 25-26 of them hold 90% of the search rows' self time
  (prof-pilot, run 1). They are deterministic let*/case/if code over values, space reads and writes on spaces rewritten constantly
  (&goal, &proof, &goal_waiter; 30-50% of their revisions remove atoms, S0), and Prolog heap calls. Each needs a
  definition guard per &self revision, which moves between statements (52-100 changes between calls for w1's top
  functions, S0). That is a guarded
  compiler for deterministic MeTTa functions (fnspec's method extended from match sites to whole functions), plus
  guards or maintained indexes on mutable search spaces. It is not a WAM-style choice machine.
- **Decision this informs.** M5 as defined should not be the next stage for PeTTaChainer. The measured lever is
  per-call interpretive overhead in deterministic code. Either a function-compilation stage (on these numbers, at
  most 1.3-2.8x on w1/w2 and 1.1-1.5x on w3) or cheaper per-call paths inside CeTTa would address it. As an example of
  the second: prepared_pure_program_cache_has_decline_head walks the prepared-program cache's entry list on a slot miss
  and is 2.6-4.9% of search self time by itself; that is a CeTTa change, outside this task.

## 6. Limits

- **One site compiled.** The method (fnspec's: keep CeTTa's row source, compile a binding-free condition and a data
  template, guard everything) reaches one hot site in these workloads; S0's second hot site (ccls_index) is a relation
  cursor plus a binding unification and stays CeTTa's (3.3), and the remainder is below the noise (3.4).
- **Where the hook sits.** Only matches that reach petta_machine_start_ready_match_mode with a space resolved at that
  point are offered (S0's match-site frames are opened there too); the first call of a run, before the space token is
  bound, is not offered (3.5); matches inside CeTTa's prepared or inlined paths do not pass there. For this site that
  leaves out one call per process (w2 263 of S0's 264 calls, w3 2,399 of 2,400).
- **The compiled site reads every row.** Its query (space_query_conjunction) may enumerate rows in another order than
  the machine's match, so it does not stop at the first passing row under once; it reads all rows (w3: 323,266 rows
  for 2,399 calls) and declines a call with more than one distinct answer (none did in w1-w3). A site that knew the
  machine's order could stop early; S1 did not build that.
- **Guards not carried over from fnspec:** type declarations of the builtins the template uses are not compared
  (their equations are); the enclosing function's own type declaration is not checked (it does not change the
  match). Revalidation is per &self program revision (equations and declarations), as fnspec.
- **Timing.** Shared host, load average 4-5 on 8 cores from other services: five interleaved runs per arm, arm order
  rotated; run-to-run ranges of 10-30% on single workloads. A 2-3% change, the site's expected end-to-end effect on
  w3, is inside that range; the profiler-row measurement (section 4.2) is the sharper one, but comes from an
  instrumented build.
- **Samples.** One SIGPROF sample per ms of process CPU time with a backtrace; CeTTa's own functions resolved with -g
  (inline chains), libc's internal functions only to the nearest exported symbol (their samples are categorised by
  the first named caller); categories are name patterns over CeTTa a9b3a811's functions
  (bench/s1/sample_categories.py) and their boundaries are approximate; the profiler's own samples (3-9%) are removed,
  but its bookkeeping still changes the instrumented build's timing.
- **The M5 estimate** combines measured shares with assumed removal fractions per mechanism (bench/s1/m5_estimate.py);
  the compiled code's own cost is taken as zero, so the high end is an upper bound; nothing was built.
- **Pins** as S0: PeTTaChainer b0e24f9b, not 33cca343 (S0 1.1).

## 7. Reproduce

```
export SWIPL_PKG_CONFIG_PATH=<swipl10 root>/share/pkgconfig GMP_PREFIX=<libgmp-dev root>/usr
bench/s0/build_s0_cetta.sh base                       # S0's base
bench/s1/build_s1_cetta.sh prof                       # + 0005 + 0006, profiler and sampler, -g
bench/s1/build_s1_cetta.sh sites                      # + 0005-0007, CETTA_S1_SITES=1, src/cpk_s1_sites.c
bench/s1/build_s1_cetta.sh sitesprof                  # the same with the profiler
bench/s1/run_bagtests.sh evidence/s1-2026-11-24/bagtest
RUNS=5 bench/s1/run_e2e.sh evidence/s1-2026-11-24/e2e
python3 bench/s1/summarize_e2e.py evidence/s1-2026-11-24/e2e [--check]
bench/s1/run_workloads.sh prof evidence/s1-2026-11-24/prof-pilot w3-benchgen w2-wmtm w1-examples
python3 bench/s1/analyze_samples.py symbolise <run>.samples.txt.gz <run>.fnprof.json --binary build/s1/cetta-prof/cetta --out <sym.json>
python3 bench/s1/step4_search.py --image build/s1/image-prof --sym W=... --fnprof W=... --out-md ... --out-json ...
python3 bench/s1/site_reach.py --classification evidence/s0-2026-11-24/classification.json --sym W=... --out-md ...
bash bench/s1/recompute_s1.sh                          # every generated table above, compared byte for byte
```

`make test` (cettapack's M1, difftest, probe and fnspec suites, on build/cetta-src without the S1 patches): EXIT=0,
every suite 0 failed (evidence/s1-2026-11-24/make-test.log); as in S0 it rewrote the commit recorded in
packs/target_expressed/certificate.metta, committed with this work.

## 8. Files

- upstream/cetta-s1/0006-fnprof-C-level-sampler-...patch, 0007-s1-sites-opt-in-compiled-match-site-hook-...patch.
- src/cpk_s1_sites.c, src/cpk_s1_sites_text.h (generated by tools/s1_sitegen.py).
- bench/s1/: build_s1_cetta.sh, run_workloads.sh, run_e2e.sh, run_bagtests.sh, bagtest/, check_routes.py,
  check_w2_goals.py, summarize_e2e.py, analyze_samples.py, sample_categories.py, step4_search.py, site_reach.py,
  recompute_s1.sh.
- evidence/s1-2026-11-24/: finding1/, bagtest/ (+ pilots), e2e/ (runs, summary.md/json), prof-pilot/ (+ gzipped raw
  samples), step4/ (tables), binaries.txt.
