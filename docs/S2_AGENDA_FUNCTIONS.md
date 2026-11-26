# S2: compiling PeTTaChainer's deterministic agenda functions under definition guards

Task t-2026-11-24-cettapack-s2-agenda-functions (goal 2026-11-24-cettapack-roadmap, stage S2, re-planned from S1;
protocosmopopos). Branch hive/t-2026-11-24-cettapack-s2-agenda-functions, **started from the S1 branch head f5bac2a,
not main** (neither S0 nor S1 is merged; S1 sits on S0 f8126a1, which sits on main efcfdd3).

Resumed 2026-11-26 10:08 (host time) after a stalled first worker: started 2026-11-24 13:33 PT, stopped about 13:49
PT without a commit or a report. Its uncommitted drafts were reviewed; what was kept, fixed or redone is in section 8.
Evidence of this run is under evidence/s2-2026-11-26/ (the draft's ranking table stays in evidence/s2-2026-11-24/rank).

## Summary

**Question.** How much of real PeTTaChainer run time can cettapack take by compiling the chainer's hot
deterministic agenda functions whole, with CeTTa's answers and with guards that hold when definitions change?

**Answer.** Little, and not measurably end to end on this host. Under the order's rules only three of S1's top 13
search functions can be compiled: frontier-remaining-score (rank 3, with its seven callees), make-and-bundle (12) and
proof-output-key-by-id (13, with two callees). Every other hot row, including four of the top five, writes the
chainer's spaces, pushes and pops its Prolog agenda heaps, changes state or collapses nondeterministic queries,
inside one mutually recursive agenda cluster; reproducing that faithfully is a different and much larger compiler,
so they stay CeTTa's. The compiled functions run inside CeTTa's own PeTTa machine through a new opt-in hook
(patch file 0008, off by default), are 2.7-4.5x faster than CeTTa on the calls they answer, and take over the work
of 7.0% / 3.7% / 2.7% of the run on w1/w2/w3 (8.8% / 5.0% / 7.0% of the search episode); the sampler measures the
saving at **5.2 / 2.3 / 2.1% of the run**. End to end (RUNS=5, interleaved, paired), on/off user time is
**0.924 (0.804-1.072) on w1, 0.944 (0.868-1.144) on w2, 0.932 (0.908-0.974) on w3**: all three medians point the
same way, w3 was faster in all five runs, but by the stated criterion none beats the host's run-to-run noise.
Peak memory falls 5% on w1 and is unchanged on w2/w3. S1's ceiling for compiling the whole search episode was
11.7-63.9% of the run saved (1.13-2.77x); S2 reaches about 2-5% (1.02-1.06x). By the order's Do 4 the stage stopped
there.

**Correctness.** All 195 end-to-end file-runs (and 52 pilot, 26 sampler runs) give S0's answers byte for byte with
no failed test. Six bag-test programs give CeTTa's answers in on and forced-invalid modes; definitions changed
mid-run in &self (an extra equation, a builtin or constructor equation, type declarations, the function replaced,
a change inside one evaluation) and in PeTTaChainer's own &proof space take the residual while changed and are
respecialised after. Two faithfulness bugs in the stalled first worker's draft were found by these tests and fixed
before any measurement (section 8).

**Open, needs a decision.** The remaining lever is the agenda cluster itself (S1's 2.8x ceiling lives there). It
needs either (a) a compiler that reproduces space writes, Prolog heap operations, state and nondeterministic
collapses in order under guards, or (b) cheaper per-call paths inside CeTTa (equation selection, call preparation,
allocation), which are CeTTa internals and need andy. Two CeTTa behaviours were found on the way and are recorded
for andy: a second `(-> Atom Number)` type declaration of expr-arity makes CeTTa not terminate, and a catch-all
equation in &proof makes it raise instantiation_error (3.4). Nothing was sent to Zar's CeTTa; the patches are files.

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
| upstream/cetta-s2/0008-s2-fns-opt-in-compiled-function-hook-at-ready-call-d.patch | 92a19e4601620d554e63be7505f66993a5a19abc5657d0dfeb7c0c0aacf6f578 |

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

| test | off = on | off = invalid | routes | verdict |
|---|---|---|---|---|
| t1_values | yes | yes | offered 47 answered 35 declined: invalid 0, call 12; invalid mode forced 124/124 | pass |
| t2_proof_store | yes | yes | offered 31 answered 25 declined: invalid 0, call 6; invalid mode forced 47/47 | pass |
| t3_redefine_self | yes | yes | offered 33 answered 13 declined: invalid 20, call 0; invalid mode forced 50/50 | pass |
| t4_redefine_module | yes | yes | offered 15 answered 7 declined: invalid 8, call 0; invalid mode forced 25/25 | pass |
| t5_proof_data | yes | yes | offered 7 answered 3 declined: invalid 0, call 4; invalid mode forced 11/11 | pass |
| t6_types | yes | yes | offered 12 answered 7 declined: invalid 0, call 5; invalid mode forced 15/15 | pass |

Every test: on and forced-invalid stdout and exit byte-identical to off (CeTTa); routes as constructed; in invalid mode every offered call forced to the residual. **0 failures.**

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

Arms (bench/s2/run_e2e.sh, RUNS=5, rounds interleaved with the arm order rotating, each round under the shared bench
lock /tmp/cettapack-bench.lock, load average 3.4-4.8 at round starts and ends, evidence/s2-2026-11-26/e2e/batches.txt):
**base** = S0's base build; **off** = the S2 fns build with CETTA_S2_FNS=0 (packs off); **on** = the same binary with
CETTA_S2_FNS=1 (packs on). S1's compiled site is switched off (CETTA_S1_SITES=0) in off and on, so on/off isolates
S2. Full tables: evidence/s2-2026-11-26/e2e/summary.md (bench/s2/summarize_e2e.py; `--check` recomputes).

### 4.1 Answers (acceptance 1)

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
| w1-examples | base | 5 | 41.46 (38.01-42.13) | 41.97 (38.47-42.62) | 153 (153-154) |
| w1-examples | off | 5 | 41.63 (37.30-42.11) | 42.15 (37.72-42.67) | 154 (153-154) |
| w1-examples | on | 5 | 38.47 (33.24-43.56) | 38.98 (33.66-44.21) | 146 (146-147) |
| w2-wmtm | base | 5 | 21.32 (20.40-23.24) | 21.39 (20.48-23.32) | 111 (109-111) |
| w2-wmtm | off | 5 | 21.68 (19.93-24.23) | 21.77 (20.00-24.31) | 111 (110-111) |
| w2-wmtm | on | 5 | 21.02 (18.84-24.32) | 21.09 (18.92-24.42) | 111 (111-112) |
| w3-benchgen | base | 5 | 65.31 (61.09-67.35) | 65.76 (61.51-67.80) | 311 (310-311) |
| w3-benchgen | off | 5 | 66.27 (58.45-71.22) | 66.60 (58.79-71.74) | 311 (310-311) |
| w3-benchgen | on | 5 | 62.87 (56.56-65.80) | 63.27 (56.98-66.24) | 309 (308-309) |

### 4.3 Paired ratios (run r of one arm against run r of another; acceptance 2)

| workload | runs | on/off user | on/off wall | off/base user | on/base user | on/off peak RSS |
|---|---|---|---|---|---|---|
| w1-examples | 5 | 0.924 (0.804-1.072) | 0.925 (0.805-1.075) | 1.011 (0.892-1.095) | 1.012 (0.795-1.034) | 0.951 (0.950-0.955) |
| w2-wmtm | 5 | 0.944 (0.868-1.144) | 0.944 (0.868-1.144) | 0.997 (0.977-1.180) | 0.953 (0.859-1.141) | 1.004 (1.000-1.010) |
| w3-benchgen | 5 | 0.932 (0.908-0.974) | 0.932 (0.908-0.976) | 1.019 (0.931-1.081) | 0.960 (0.857-1.008) | 0.992 (0.990-0.999) |

### 4.4 Does the gain beat run-to-run noise?

Criterion: every paired on/off user ratio < 1, and median on/off < 1 - max(|off/base - 1|) (the gain is larger than the largest difference between two builds of the same code).

| workload | on/off user, every run < 1 | median on/off | noise floor 1 - max abs(off/base - 1) | beats noise |
|---|---|---|---|---|
| w1-examples | no | 0.924 | 0.892 | **no** |
| w2-wmtm | no | 0.944 | 0.820 | **no** |
| w3-benchgen | yes | 0.932 | 0.919 | **no** |

Plainly: **the gain does not beat run-to-run noise on any workload by the stated criterion.** The medians point the
same way on all three (6-8% less user time with the functions on), and on w3 all five paired runs were faster
(0.908-0.974; a sign test over five runs gives p = 1/32), but two builds of the same code (off and base) differ by up
to 8% on w3 and 11-18% on w1/w2 run to run, which is as large as the effect. The size the coverage supports is
smaller still (section 5): the sampler measures what compiling these functions removes at 5.2 / 2.3 / 2.1% of
the run (w1/w2/w3), so the w2 and w3 medians (5.6%, 6.8%) are larger than the compiled code can explain, which again
says noise dominates; w1's median (7.6%) is near its expected 5.2%. Peak
memory falls 5% on w1 (146 vs 154 MiB, every run) and is unchanged on w2 and w3.

### 4.5 The compiled functions in the on arm (counters summed over the workload's files; identical in all 5 runs)

| workload | runs | function | offered | answered | declined: guard invalid | declined: call outside the code | same in every run |
|---|---|---|---|---|---|---|---|
| w1-examples | 5 | proof-output-key-by-id | 30282 | 30282 | 0 | 0 | yes |
| w1-examples | 5 | proof-output-atom-by-id | 21899 | 21899 | 0 | 0 | yes |
| w1-examples | 5 | and-bundle-premises | 19307 | 19307 | 0 | 0 | yes |
| w1-examples | 5 | premises-expected-confidence | 18836 | 5854 | 0 | 12982 | yes |
| w1-examples | 5 | term-confidence | 13738 | 1462 | 0 | 12276 | yes |
| w1-examples | 5 | make-and-bundle | 13057 | 13057 | 0 | 0 | yes |
| w1-examples | 5 | premises-expected-confidence-step | 12982 | 0 | 0 | 12982 | yes |
| w1-examples | 5 | tv-confidence | 11457 | 11457 | 0 | 0 | yes |
| w1-examples | 5 | candidate-output-key | 6633 | 6633 | 0 | 0 | yes |
| w1-examples | 5 | frontier-remaining-score | 6529 | 1214 | 0 | 5315 | yes |
| w1-examples | 5 | frontier-pool-potential | 5376 | 5376 | 0 | 0 | yes |
| w2-wmtm | 5 | and-bundle-premises | 8269 | 8269 | 0 | 0 | yes |
| w2-wmtm | 5 | premises-expected-confidence | 7827 | 2074 | 0 | 5753 | yes |
| w2-wmtm | 5 | proof-output-key-by-id | 7446 | 7446 | 0 | 0 | yes |
| w2-wmtm | 5 | proof-output-atom-by-id | 6584 | 6584 | 0 | 0 | yes |
| w2-wmtm | 5 | make-and-bundle | 6398 | 6398 | 0 | 0 | yes |
| w2-wmtm | 5 | premises-expected-confidence-step | 5753 | 0 | 0 | 5753 | yes |
| w2-wmtm | 5 | term-confidence | 5717 | 403 | 0 | 5314 | yes |
| w2-wmtm | 5 | tv-confidence | 4731 | 4731 | 0 | 0 | yes |
| w2-wmtm | 5 | frontier-remaining-score | 2018 | 203 | 0 | 1815 | yes |
| w2-wmtm | 5 | frontier-pool-potential | 1815 | 1815 | 0 | 0 | yes |
| w2-wmtm | 5 | candidate-output-key | 1796 | 1796 | 0 | 0 | yes |
| w3-benchgen | 5 | premises-expected-confidence | 19322 | 4782 | 0 | 14540 | yes |
| w3-benchgen | 5 | premises-expected-confidence-step | 14540 | 0 | 0 | 14540 | yes |
| w3-benchgen | 5 | tv-confidence | 14304 | 14304 | 0 | 0 | yes |
| w3-benchgen | 5 | term-confidence | 14176 | 3252 | 0 | 10924 | yes |
| w3-benchgen | 5 | and-bundle-premises | 10029 | 10029 | 0 | 0 | yes |
| w3-benchgen | 5 | proof-output-atom-by-id | 8838 | 8838 | 0 | 0 | yes |
| w3-benchgen | 5 | make-and-bundle | 7386 | 7386 | 0 | 0 | yes |
| w3-benchgen | 5 | proof-output-key-by-id | 5159 | 5159 | 0 | 0 | yes |
| w3-benchgen | 5 | frontier-remaining-score | 2643 | 339 | 0 | 2304 | yes |
| w3-benchgen | 5 | candidate-output-key | 2462 | 2462 | 0 | 0 | yes |
| w3-benchgen | 5 | frontier-pool-potential | 2382 | 2382 | 0 | 0 | yes |

### Decline reasons (on arm, first run, summed over files)

| workload | reason | calls |
|---|---|---|
| w1-examples | premises-expected-confidence-step: CPU premise (formula evaluation not compiled) | 31279 |
| w1-examples | term-confidence: CPU term (formula evaluation not compiled) | 12276 |
| w2-wmtm | premises-expected-confidence-step: CPU premise (formula evaluation not compiled) | 13321 |
| w2-wmtm | term-confidence: CPU term (formula evaluation not compiled) | 5314 |
| w3-benchgen | premises-expected-confidence-step: CPU premise (formula evaluation not compiled) | 31384 |
| w3-benchgen | term-confidence: CPU term (formula evaluation not compiled) | 10924 |

Recognised calls by reason (S2 counters, CETTA_S2_FNS_STATS per file): every decline in the runs is a call outside
the compiled code (CPU premises and CPU terms: frontier-remaining-score answers 18.6% / 10.1% / 12.8% of its calls on
w1/w2/w3); no guard was invalid in any workload run (the chainer does not redefine these functions); the counters
are identical in all five runs.


## 5. Coverage, and why the stage stops (order Do 3-4)

### 5.1 Measured with the sampler (fnsprof build, one run per workload and arm, under the bench lock)

bench/s2/run_sprof.sh, bench/s2/compiled_samples.py (evidence/s2-2026-11-26/sprof/samples_compiled.md; every
sampler run's stdout = S0's, 26/26). One SIGPROF sample per ms of CPU time; shares of samples without the profiler's
own:

| workload | arm | samples | search rows | compiled code | compiled, of search | fn rows (CeTTa) | compiled + fn rows |
|---|---|---|---|---|---|---|---|
| w1-examples | off | 46522 | 80.43% | 0.03% | 0.03% | 13.78% | 13.82% |
| w1-examples | on | 45776 | 78.73% | 1.81% | 2.17% | 6.74% | 8.55% |
| w2-wmtm | off | 24777 | 73.95% | 0.03% | 0.02% | 8.75% | 8.77% |
| w2-wmtm | on | 26198 | 75.14% | 1.39% | 1.80% | 5.02% | 6.41% |
| w3-benchgen | off | 83819 | 39.14% | 0.03% | 0.03% | 5.19% | 5.22% |
| w3-benchgen | on | 78580 | 36.82% | 0.61% | 1.16% | 2.45% | 3.06% |

Derived (same samples; "handed to compiled code" = the off arm's CeTTa time in the functions' rows minus the on
arm's residual, i.e. the CeTTa work the compiled code now does, as a share of the off arm's search-episode
samples):

| workload | off: CeTTa in the functions' rows | on: residual calls | on: compiled code | on: both | saved (of run) | compiled code vs CeTTa on the calls it answers | handed to compiled code | S1 ceiling (whole episode, saved of run) |
|---|---|---|---|---|---|---|---|---|
| w1-examples | 13.8% | 6.7% | 1.8% | 8.6% | 5.2 points | 3.9x | 8.8% of search | 24.3-63.9% (1.32-2.77x) |
| w2-wmtm | 8.7% | 5.0% | 1.4% | 6.4% | 2.3 points | 2.7x | 5.0% of search | 21.5-59.1% (1.27-2.45x) |
| w3-benchgen | 5.2% | 2.5% | 0.6% | 3.1% | 2.1 points | 4.5x | 7.0% of search | 11.7-31.3% (1.13-1.46x) |

So: compiled code now does the work of **7.0% / 3.7% / 2.7% of the run** (w1/w2/w3), **8.8% / 5.0% / 7.0% of the
search episode**, at 2.7-4.5x CeTTa's speed for those calls, which removes **5.2 / 2.3 / 2.1 points** of the
instrumented run. Against S1's ceiling for compiling the whole search episode (11.7-63.9% of the run saved,
1.13-2.77x), S2 reaches 2-5% (about 1.02-1.06x), roughly a tenth of the ceiling's lower end on w1/w2 and a fifth on w3.
That expected saving is smaller than the end-to-end noise (section 4.4), which is why the end-to-end medians
(6-8%) cannot be taken at face value.

### 5.2 Estimate from S1's profile and S2's counters (cross-check)

bench/s2/coverage.py (evidence/s2-2026-11-26/coverage/coverage.md): S1's CeTTa self time per compiled function x the
fraction of its calls S2 answered (e2e on arm, run 1). Estimated share handed to compiled code: 8.2% / 4.5% / 2.9% of
the run (10.2% / 6.0% / 7.7% of search self time), against 7.0% / 3.7% / 2.7% measured in 5.1: the two agree within
about a point.

### 5.3 Why the stage stops (order Do 4)

The order: after the top five, if the end-to-end gain is still within noise, stop compiling and say why. Of the top
five, one (rank 3) can be compiled under the order's rules; with it and ranks 12-13 compiled, the end-to-end gain
is within noise (pilot, RUNS=2: on/off 0.89-1.09; RUNS=5: 4.4). Why:

1. **The residual dominates the hot list.** 17 of the top 20 rows are declined, holding 66.9% of mean search self
   time (the top 20 hold 80.0%; the three compiled rows 13.1%). One is S1's ccls_index match site; the other 16 are
   the agenda cluster: functions that write the chainer's spaces (&goal, &goal_waiter, &proof, caches), push and
   pop the Prolog agenda heaps, change state, or collapse nondeterministic queries, and call each other. The order
   declines what cannot be reproduced faithfully; reproducing those effects in order, under guards, is a different
   and much larger compiler (section 6).
2. **The compilable functions are cheap rows.** All compiled functions together are 13.9% / 9.1% / 5.6% of the run in
   CeTTa (S1 profile), and frontier-remaining-score, the largest, can answer only 10-19% of its calls: the others
   have CPU premises whose expected confidence is a nondeterministic TV-formula evaluation (cpu-expected-tv).
3. **Call boundary and guards are not the problem:** the compiled code is 2.7-4.5x faster than CeTTa on the calls it
   answers, including the hook, admission check, guard token compare and operand observation; off/base shows the hook
   switched off costs nothing measurable (4.3: 1.011, 0.997, 1.019).


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
- **Coverage** is measured with the sampler on an instrumented build, one run per arm (5.1), and cross-checked by an
  estimate from S1's profile and S2's counters (5.2); they agree within about a point.
- **Timing.** Shared host (8 cores, load 3.5-5.6 during the runs, another cettapack worker benchmarking under the same
  lock); five interleaved runs per arm, arm order rotating; paired ratios.

## 7. Ledger of checks (sanity gate)

| check | expected | observed | evidence |
|---|---|---|---|
| known-answer canary, per file-run | stdout byte-identical to S0's base run 1, exit 0, no failed test | 195/195 (RUNS=5), 52/52 (pilot), sampler runs: see sprof | e2e/*/W/*.check, e2e/summary.md |
| per-unit structural check | check line written per run; tables use only "same" runs | written for every run; no DIFFERENT line | bench/s2/run_workloads.sh |
| compiled code actually ran (not a silent no-op) | answered > 0 for each compiled entry on every workload | answered counts in 4.5, identical in 5 runs | e2e/on/W/*.fns.json |
| guards not silently failing | invalid mode forces every offered call; redefinitions invalidate and respecialise | bag tests 6/6 | bagtest/summary.md, *.stats.json |
| hook reachability | the hook is reached in real runs | first build: 0 offered (hook after the relation path), found and fixed | bagtest-pilot (pilot 1), section 2 |
| faithfulness probes vs CeTTa | quote, stored outputs, decl changes | quote bug found and fixed; outputs returned verbatim; two CeTTa non-terminations/errors recorded | probes/, bagtest-pilot2/3 |
| pilot before long run | RUNS=2 off/on before RUNS=5 | answers identical; within noise (Do 4 decision) | e2e-pilot/ |
| shared-host discipline | every timed round under /tmp/cettapack-bench.lock, load logged | 5 e2e rounds + 2 pilot rounds + sampler rounds | e2e/batches.txt, e2e-pilot/batches.txt, sprof/batches.txt |
| binaries | the runs used the recorded builds | fns 959d1c2d..., base 4b7c08c4... in every env file | binaries.txt, e2e/*/env-*.txt |
| make test | EXIT=0, every suite 0 failed | EXIT=0, 76 suites 0 failed | make-test.log |

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
