# S2: compiling PeTTaChainer's deterministic agenda functions under definition guards

Task t-2026-11-24-cettapack-s2-agenda-functions (goal 2026-11-24-cettapack-roadmap, stage S2, re-planned from S1;
protocosmopopos). Branch hive/t-2026-11-24-cettapack-s2-agenda-functions, **started from the S1 branch head f5bac2a,
not main** (neither S0 nor S1 is merged; S1 sits on S0 f8126a1, which sits on main efcfdd3).

Resumed 2026-11-26 10:08 (host time) after a stalled first worker: started 2026-11-24 13:33 PT, stopped about 13:49
PT without a commit or a report. Its uncommitted drafts were reviewed; what was kept, fixed or redone is in section 8.
Evidence of this run is under evidence/s2-2026-11-26/ (the draft's ranking table stays in evidence/s2-2026-11-24/rank).

@@SUMMARY@@

## 1. Inputs and pins

| what | pin | note |
|---|---|---|
| cettapack | start f5bac2a (S1 head) | not main; S0 f8126a1 on main efcfdd3 |
| CeTTa | a9b3a811 + patch files 0001-0004 (upstream/cetta), 0005 (cetta-s0), 0006-0007 (cetta-s1), **0008 (cetta-s2, this stage)** | files only; nothing sent to Zar's CeTTa |
| PeTTaChainer | b0e24f9b (pettachainer/metta subtree) | as S0/S1 |
| PeTTa | e038e4db, lib/lib_*.metta overlaid | as S0/S1 |
| SWI-Prolog | 10.0.2 user-local (scratch/deps/swipl10) | as S0/S1 |
| workloads | S0's w1-w3, unchanged (bench/s0/gen_workloads.py, seed 20261124) | |

Patch files (sha256):

| file | sha256 |
|---|---|
| upstream/cetta/0001-embedding-in-process-native-borrowing-for-compiled-k.patch | 6cdc7715079ea6e0d0489e25d86569166d6c7643027a34274176cf4b3f2b465c |
| upstream/cetta/0002-mork-bridge-bulk-prefix-lookup-mork_space_prefix_val.patch | 9b8d6465e690a695a962aee7c7bf364472bc4cb29549fa8383d4873dfe04052c |
| upstream/cetta/0003-eval-match-rows-direct-entry-to-the-conjunction-matcher.patch | 1efd7acd5142311fc7f86257be3920dfbceb7c63e6fab84430711b68d8b9d49a |
| upstream/cetta/0004-match-rows-row-hooks-compiled-code-inside-the-matcher.patch | 4d8970c7776f2afc959c86fc84fc54904d85143c0a81b802abe78e7bb39e5d59 |
| upstream/cetta-s0/0005-fnprof-opt-in-PeTTa-function-profiler-CETTA_FNPROF-1.patch | f2019a5cfbedde455109e32378f87f0b670a8bce7f419e56eaa367db7af24317 |
| upstream/cetta-s1/0006-fnprof-C-level-sampler-CETTA_FNPROF_SAMPLE_US-sample.patch | 2de0d29f411e6f3c07116ae07b2f6690fdeeb7398d33a4b147e8e48ea01c1494 |
| upstream/cetta-s1/0007-s1-sites-opt-in-compiled-match-site-hook-in-PeTTa-s-.patch | c1c5af23196f022ac3e435b77a64d24217398c6ae6f61be82eccc0bc82d9a438 |
| upstream/cetta-s2/0008-s2-fns-opt-in-compiled-function-hook-at-ready-call-d.patch | @@P0008@@ |

Builds (bench/s2/build_s2_cetta.sh; sha256 in evidence/s2-2026-11-26/binaries.txt): `fns` = 0001-0008 with
CETTA_S1_SITES=1 CETTA_S2_FNS=1 and cettapack's src/cpk_s1_sites.c and src/cpk_s2_fns.c built in (packs off/on is the
run-time switch $CETTA_S2_FNS of this one binary; S1's site is switched off, CETTA_S1_SITES=0, in every S2 run);
`fnsprof` = the same with CETTA_FNPROF=1 and -g (sampler runs only). `base` = S0's base build (0001-0004), reused
from the S1 worktree (sha256 recorded per run).

Sanity gate (/home/openclaw/research-agent/docs/EXPERIMENT_SANITY_GATE.md): known-answer canary = S0's recorded
stdout of every workload file; every S2 run writes a check line (exit status, stdout sha256, equal to S0's or not,
failed tests; bench/s2/run_workloads.sh) and the tables use only runs whose line says "same". Pilots before the long
runs: bag-test pilots (evidence/s2-2026-11-26/bagtest-pilot, -pilot2, -pilot3: what they found is in 3.4), the
toothbrush file on/off (3.1), and the end-to-end pilot (evidence/s2-2026-11-26/e2e-pilot, RUNS=2). Ledger of checks:
section 7.

## 2. The hook (patch file 0008) and how it was interpreted

The order says "use S1's in-machine hook"; S1's 0007 intercepts match starts only, and compiling whole functions
needs control at function-call entry. The dispatcher's interpretation, followed here: allowed is a new opt-in,
off-by-default patch file of the same kind as 0007 that adds only a hook point at function-call entry in PeTTa's
search machine, through which compiled code (built into CeTTa from cettapack source) answers the call or declines to
the residual; with the switch off nothing changes and nothing compiled is called. Not allowed: changing CeTTa's
evaluator semantics, matcher, data structures, caches, allocator or prepared-call cache.

**upstream/cetta-s2/0008** (build flag CETTA_S2_FNS=1, default 0; run-time switch $CETTA_S2_FNS: 0 off, 1 on,
`invalid` = every guard forced invalid). In petta_machine_dispatch_goal, for a ready call goal
(PETTA_GOAL_CALL_READY / _RESOLVED: operands are values) whose head a compiled function claims, and **only when
CeTTa's own admission for its relation path (host admit_space_query_values, the check
petta_machine_try_ready_relation_invocation makes) says CeTTa would run this call over the program's equations at
this point**, the call (operands with the machine's bindings applied) is offered to cetta_s2_fns_try. An answer is
unified with the goal's expected result, as the relation path and CeTTa's own ready-value fast paths do
(petta_machine_unify); a decline changes nothing and the call runs as before. The admission gate means a call CeTTa
would route anywhere else (a Prolog predicate, a memo or tabled relation, a translator rule, an arrow signature, a
specializer route, or a machine in analysis/capture mode) is never answered by compiled code.

Found on the way (bag-test pilot 1): placing the hook *after* the relation path made it dead. In CeTTa's PeTTa mode,
ordinary function calls with equations run *through* that path (an equation query, the open-equation cursor), so
it must sit before it, gated by the same admission.

## 3. What was compiled, and what was declined (order Do 1)

### 3.1 S1's ranked list, and the decision per function

The ranking (bench/s2/rank_functions.py over S1's prof-pilot samples; evidence/s2-2026-11-24/rank/rank.md) pools
w1-w3 with equal weight; 31 rows hold 90% of mean search self time (S1 counted 25-26 per workload). The order's rule:
compile in order of time; decline a function that is nondeterministic, does side effects that cannot be reproduced
faithfully, or calls something whose meaning cannot be guarded, and move on.

| rank | row | mean % of search self | decision | why |
|---|---|---|---|---|
| 1 | advance-frontier-waiter-heap | 11.6 | declined | collapse over the nondeterministic frontier-premise-part; calls spawn-frontier-heap / frontier-advance-pooled-heap, which write &goal_waiter, &goal, &goal_lookup and push the Prolog agenda heap; mutually recursive with 2, 4, 5 through proof-fold-flat-semidet |
| 2 | spawn-frontier-goal-heap | 8.0 | declined | chainer-add-atom &goal_waiter, then a fold over advance-frontier-waiter-heap (1), then schedule-goal-heap (Prolog heap) |
| 3 | frontier-remaining-score | 7.4 | **compiled** | deterministic, writes nothing; with callees and-bundle-premises, premises-expected-confidence(-step), term-confidence, proof-score, tv-confidence, frontier-pool-potential, expr-arity |
| 4 | commit-proof-candidate-heap | 5.3 | declined | store-proof-candidate (writes &proof, proof-id state), wake-proof-commit-heap (agenda cluster) |
| 5 | spawn-frontier-heap | 5.3 | declined | ensure-goal-at-depth (writes &goal, &goal_lookup, gid state), commit-proof-candidate-heap (4), spawn-frontier-goal-heap (2) |
| 6 | match@rule-goal-full-view ccls_index | 4.3 | declined | S1 3.3: a PeTTa dynamic relation cursor plus a binding unification |
| 7 | schedule-goal-row | 4.2 | declined | pushes onto the Prolog agenda heap |
| 8 | rule-expected-score | 4.1 | declined | reads rule_expected, a PeTTa dynamic relation (bare-symbol, Prolog side) the guard cannot read |
| 9 | goal-merged-proof-ids | 4.0 | declined | goal-live-proof-ids writes the live-proof cache (goal-live-cache-store!) |
| 10 | ensure-goal-at-depth | 3.4 | declined | writes &goal, &goal_lookup, next-gid state |
| 11 | search-agenda-heap | 3.2 | declined | the agenda loop: Prolog heap pops, all of the above |
| 12 | make-and-bundle | 2.9 | **compiled** | a constructor call |
| 13 | proof-output-key-by-id | 2.8 | **compiled** | with proof-output-atom-by-id (a once over a match on &proof) and candidate-output-key |
| 14 | advance-frontier-proof-waiters-heap | 2.5 | declined | agenda cluster (1) |
| 15 | expand-direct-goal-step | 1.9 | declined | commit-proof-candidate-heap (4) |
| 16 | search-agenda-step | 1.9 | declined | remove-atom on the agenda spaces |
| 17 | chainer-run-many | 1.8 | declined | change-state! |
| 18 | expand-goal-heap | 1.8 | declined | chainer-add-atom / remove-atom |
| 19 | goal-live-proof-ids-for-output-key | 1.7 | declined | goal-live-proof-ids cache writes (9) |
| 20 | proof-fold-flat-semidet | 1.7 | declined | higher-order fold whose step is the agenda cluster's functions |

Of the top five, one (rank 3) is compilable under the order's rule; ranks 12 and 13 are the next ones that are. Every
other row in the top 20 writes to the chainer's spaces, its Prolog heaps or its state, directly or through the
mutually recursive agenda cluster (1, 2, 4, 5, 14, 15, 20 call each other). Compiling that cluster faithfully means
reproducing its space writes, Prolog heap operations and state changes in order, i.e. reimplementing most of
backward_chainer.metta in C; under the order that is declined, and it is why the stage stops (Do 4, section 5).

### 3.2 The compiled code (src/cpk_s2_fns.c, texts src/cpk_s2_fns_text.h by tools/s2_fngen.py)

Each compiled function computes CeTTa's answer or declines the whole call; none writes anything, so declining at any
point is the residual. Callees are compiled into their callers (no hook round trip) and are also entry points of
their own. Per call:

- `case`/head unification: where CeTTa would bind a variable of the value (a variable or variable-headed term where
  a pattern has a constructor), the call declines; numbers go through CeTTa's own scalar operations
  (petta_numeric_min_max, grounded_try_plain_scalar_arithmetic/_truth) and a non-number declines;
- CPU premises and CPU terms decline: their expected confidence is cpu-expected-tv, a nondeterministic collapse over
  the PLN TV formulas (CTVModusPonensFormula, AndFormula, ...), which the code does not reproduce;
- copy_term in frontier-remaining-score is not built: it only protects the caller's variables from bindings the
  compiled code never makes, and the answer is a number;
- candidate-output-key builds `(proof-output-key ($type $kb))`, **without quote**: CeTTa's PeTTa evaluates
  `(quote X)` as an argument to X (probe in bag test t1; the stalled draft built the quote and would have changed
  answers);
- proof-output-atom-by-id queries &proof live on every call (space_query_conjunction) and answers only when the rows
  for the pid give exactly one output up to renaming (once takes the matcher's first row, whose order the compiled
  query does not reproduce); the output is answered **as stored, renamed apart** (fresh variables): CeTTa returns
  match's $output verbatim, without evaluating any part of it, even parts with equations or grounded heads (probe
  evidence/s2-2026-11-26/probes/nf.*; bag test t2 checks it per mode). The stalled draft required a ground normal
  form, which declined 90% of real calls (stored proofs are CPU terms over formula symbols).

### 3.3 Guards (order Do 2, acceptance 3)

| guard | checked | failing -> |
|---|---|---|
| forced invalid ($CETTA_S2_FNS=invalid) | every call | residual |
| route: CeTTa's own relation-path admission for this call is "run over the equations" | every call (0008) | residual |
| definition: in &self, the equations of the function and of every function its code reproduces (closed under calls) are exactly the compiled ones (count, order, alpha-equal), with the same type declaration; no catch-all equation | per program revision (space_program_token: equation and declaration revisions) of every space read | residual until a revision moves |
| builtin meanings: no equation in &self for any builtin the code reproduces (if case let let* == < + * min max is-var cdr-atom copy_term once match quote cons collapse) | as above | residual |
| constructors: no equation for any constructor the code builds or dispatches on (and-bundle proof-output-key STV CTV CPU pool-part marginal-proj stored-proof-row), and their type declarations are the compiled ones | as above | residual |
| other spaces read: &proof (PeTTaChainer's proof store) and every module dependency of &self: no equation for a compiled head or constructor, no catch-all | per revision of each | residual |
| values: operands of the shapes the code covers (4.2) | every call | that call declines |

Module spaces. CeTTa links HE-style imports as dependency spaces of the importer (library_link_he_module), and the
guard tracks each dependency's revision; but in these runs a PeTTa `import!` of the chainer loads its modules into
&self itself (the guard's trace shows 0 dependencies of &self in every bag test and workload), so PeTTaChainer's own
spaces are its data spaces (&proof, &goal, &goal_waiter, ...). The compiled code reads &proof; t4 redefines
compiled functions inside &proof. CeTTa does not use equations stored in &proof to evaluate calls (its answers do not
change, t4's off output), so that guard is conservative: the residual is taken while such a definition exists.

### 3.4 Bag tests (acceptance 1 and 3)

bench/s2/run_bagtests.sh: six PeTTa programs (bench/s2/bagtest/t1-t6, each prefixed by common.inc, which imports
PeTTaChainer), each run in its own process in three modes on the fns build: off (CeTTa's answers), on, forced
invalid. Pass = on and invalid stdout and exit byte-identical to off, and the counters as each test's construction
predicts (bench/s2/bagtest/expected.json, bench/s2/check_routes.py; in invalid mode every offered call is forced to
the residual). Results: evidence/s2-2026-11-26/bagtest/summary.md.

@@BAGTABLE@@

What they cover: every compiled entry on values of each shape it covers and on shapes where it declines (variables
a case would bind, CPU terms and premises, non-numbers, non-lists; t1); proof-store lookups: one row, no row, two rows
with one output, two rows with different outputs, an output with variables (compared across two calls with == and
=alpha, and destructured), outputs holding formula symbols, symbols and calls with equations and grounded heads (t2);
mid-run changes in &self, each undone: a second equation of a compiled callee (two answers), an equation for a
builtin the code reproduces (min), an equation for a constructor it builds (and-bundle), a second type declaration of
a compiled callee (expr-arity), a type declaration for a constructor it dispatches on (STV), the function's equation
replaced by a different one, and a redefinition inside one evaluation (t3); definitions of a compiled function and
of a compiled callee placed in PeTTaChainer's &proof, then removed, and one in &goal, a chainer space the code does
not read, which leaves it specialised (t4); &proof data changing mid-run (a second row with a different output: the
order guard declines; removed: specialised again; t5); ill-typed operands, calls inside let*/case/collapse, and an
expected result already bound (t6). In every change block the call takes the residual while the change holds and
is answered by compiled code again after the undo (the trace lines are in the *.on.stderr files when run with
TRACE=3).

Found by the pilots and fixed or recorded:
- bare top-level calls `!(f ...)` of some of these functions take a CeTTa route that does not pass the machine's
  ready-call dispatch (never offered); calls inside a wrapper that copies its operand (copy_term) are, so t3/t4 call
  through such wrappers;
- removing an added type declaration that unifies with the original (`(-[det]-> Expression Number)` vs the
  original `(-[det]-> $expression Number)`) removes both; the guard then rightly stays invalid (the definition did
  change); the test uses a declaration that cannot unify;
- **CeTTa itself does not terminate** (in every mode, also S0's base build) after `(: expr-arity (-> Atom Number))`
  is added and expr-arity is called (evidence/s2-2026-11-26/probes/t3_c2.*); and a catch-all equation
  `(= ($f $x) ...)` in &proof makes CeTTa raise instantiation_error. Neither is usable as a test; both are CeTTa
  behaviour, recorded for andy.

## 4. End to end (order Do 3, acceptance 1-2)

@@E2E@@

## 5. Coverage, and why the stage stops (order Do 3-4)

@@COVERAGE@@

## 6. Limits

- **Three of the top 13 functions compiled, one of the top five.** The order's rule declines everything that writes
  spaces, Prolog heaps or state; on this chainer that is the whole agenda cluster, which is where S1's ceiling lives.
  The remaining lever needs either a compiler that reproduces those effects in order (space writes, Prolog heap
  calls, state, nondeterministic collapses) under guards, or cheaper per-call paths inside CeTTa; both are outside
  this order.
- **frontier-remaining-score answers 10-19% of its calls**: CPU premises need cpu-expected-tv (TV formulas,
  nondeterministic collapse), not compiled.
- **Route-dependent offering.** Only calls that reach the machine's ready-call dispatch with CeTTa's relation
  admission are offered; calls CeTTa runs on other routes (some top-level calls; expr-arity in these workloads) are
  never offered. Coverage counts only what was answered.
- **Guards not carried:** type declarations of the builtins are not compared (their equations are); the guard is
  global per revision (any invalid dependency or foreign definition sends every dependent function to the residual
  until the revision moves); a definition in &proof is guarded conservatively.
- **The coverage share is an estimate** (5.1) combining S1's profile with S2's counters; the sampler measurement
  (5.2) is from an instrumented build.
- **Timing.** Shared host (8 cores, load 3.5-5.6 during the runs, another cettapack worker benchmarking under the same
  lock); five interleaved runs per arm, arm order rotating; paired ratios.

## 7. Ledger of checks (sanity gate)

@@LEDGER@@

## 8. The stalled worker's drafts

| draft | verdict |
|---|---|
| bench/s2/rank_functions.py, evidence/s2-2026-11-24/rank | kept (reads S1's samples; the ranking above) |
| tools/s2_fngen.py, src/cpk_s2_fns_text.h | kept (verbatim texts, sha256 of the extracted forms in the header) |
| src/cpk_s2_fns.c | kept after review, fixed: candidate-output-key's quote (would have changed answers); proof outputs answered as stored, renamed apart (the ground normal-form requirement declined 90% of real calls); constructor declarations and module dependency spaces added to the guard |
| hook draft in build/s2/cetta-dev (not a patch file) | redone as upstream/cetta-s2/0008: debug probe removed, gated on CeTTa's relation admission |
| bench/s2/build_s2_cetta.sh, run_workloads.sh, check_routes.py | kept |
| bench/s2/run_e2e.sh | redone: bench lock per round, load average logged |
| bench/s2/run_bagtests.sh, bagtest/t1-t4 | kept t1, t2 (extended); t3, t4 rewritten (wrappers, CeTTa-runnable changes); t5, t6, expected.json new |

## 9. Reproduce

```
export SWIPL_PKG_CONFIG_PATH=<swipl10 root>/share/pkgconfig GMP_PREFIX=<libgmp-dev root>/usr
bench/s2/build_s2_cetta.sh fns                     # 0001-0008 + compiled functions, CETTA_S2_FNS=1
bench/s2/build_s2_cetta.sh fnsprof                 # the same with the profiler and -g
bench/s2/run_bagtests.sh evidence/s2-2026-11-26/bagtest
RUNS=5 ARMS="base off on" bench/s2/run_e2e.sh evidence/s2-2026-11-26/e2e
python3 bench/s2/summarize_e2e.py evidence/s2-2026-11-26/e2e
python3 bench/s2/coverage.py --s1-prof evidence/s1-2026-11-24/prof-pilot --s1-step4 evidence/s1-2026-11-24/step4/step4.json \
  --e2e evidence/s2-2026-11-26/e2e --out-md evidence/s2-2026-11-26/coverage/coverage.md --out-json evidence/s2-2026-11-26/coverage/coverage.json
bash bench/s2/recompute_s2.sh                       # every generated table, compared byte for byte
```
