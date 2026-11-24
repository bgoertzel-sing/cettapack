# S1: compiling the hot PeTTaChainer sites S0 found, and sizing the search-episode lever

Task t-2026-11-24-cettapack-s1-hot-sites (goal 2026-11-24-cettapack-roadmap, stage S1, re-planned from S0;
order projects/cettapack/orders, protocosmopopos). Branch hive/t-2026-11-24-cettapack-s1-hot-sites.

## Summary

@@SUMMARY@@

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

@@REACH@@

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

@@BAGTEST@@

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

@@E2E@@

## 5. Search-episode sizing (step 4, report only)

@@STEP4@@

## 6. Limits

@@LIMITS@@

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

## 8. Files

- upstream/cetta-s1/0006-fnprof-C-level-sampler-...patch, 0007-s1-sites-opt-in-compiled-match-site-hook-...patch.
- src/cpk_s1_sites.c, src/cpk_s1_sites_text.h (generated by tools/s1_sitegen.py).
- bench/s1/: build_s1_cetta.sh, run_workloads.sh, run_e2e.sh, run_bagtests.sh, bagtest/, check_routes.py,
  check_w2_goals.py, summarize_e2e.py, analyze_samples.py, sample_categories.py, step4_search.py, site_reach.py,
  recompute_s1.sh.
- evidence/s1-2026-11-24/: finding1/, bagtest/ (+ pilots), e2e/ (runs, summary.md/json), prof-pilot/ (+ gzipped raw
  samples), step4/ (tables), binaries.txt.
