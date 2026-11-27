# cetta-internals P: where CeTTa's evaluator spends its time on PeTTaChainer, and which changes would cut it

Task t-2026-11-27-cetta-internals-p-profile (goal 2026-11-27-cetta-internals, Ben, Work msg 918). Staged cettapack
branch hive/t-2026-11-27-cetta-internals-p-profile, base **3f15d5d** (the native-primitives head = main efcfdd3 + S0 +
S1 + np, which carries the profiler tooling; none of them is merged into main yet). Nothing was sent to Zar's CeTTa.

## Summary

On w1-w3, CeTTa's evaluator spends its time in four places of similar size: the goal-dispatch loop (32-33% with
choice handling and prepared execution), equation lookup, selection and call preparation (24-28%), term allocation
and copying (20%), and unification/matching (9-14%); space access (5-6%) and Prolog (4-6%) are small. Measured with
new opt-in timers on 112 evaluator functions (patch 0011), overhead calibrated and subtracted, answers byte-identical
to S0 in every run; S1's sampler agrees within 1.5 points per phase except allocation (it also sees libswipl's).

Four general changes, ranked by estimated end-to-end gain: **C2** admit the prepared-call fast path only where it
pays (measured: turning it off already saves 8-11%), **C1** cache each call's preparation decision per call site with
per-head invalidation (assumed 9-10%), **C3** a faster allocator, then pools (measured with mimalloc: 8-10%), **C4**
share instead of re-instantiate ground/settled terms (assumed 4%). Together about 31-33% saved, **1.45-1.49x** (1.37-1.56x
with C2 and C3 taken from their joint measurement), with C2 and C3 together measured at 0.78 / 0.86 / 0.82 of the base's user time (1.28x / 1.16x / 1.22x) with no code change. **Go/no-go: 1.5x is plausible but
borderline (it needs C1's assumed 60% to hold); 2x is not**: the four changes touch 44-49% of the run, and the rest
(dispatch loop, matching) is diffuse interpretation that only compiling the agenda functions reaches.

## 1. Setup

- **CeTTa** a9b3a81186529fd99649e437809bdb6f58fce2fd + upstream/cetta/0001-0004 (unchanged) + new patch file
  **upstream/cetta-p/0011-evprof-opt-in-evaluator-phase-timers-CETTA_EVPROF-1-.patch**. Calibration prototype (Do 4):
  **upstream/cetta-p-proto/0012-eval-index-the-prepared-pure-program-cache-by-call-k.patch**. sha256 of every patch
  and binary: evidence/p-2026-11-27/binaries.txt. Builds: bench/p/build_p_cetta.sh (off, ev, stats, samp, and
  off/ev + 0012; all -O3 -g; SWI-Prolog 10.0.2 interop as in S0).
- **Workloads**: S0's w1-w3 unchanged (PeTTaChainer b0e24f9b, PeTTa e038e4db lib overlay; w1 = 11 PeTTaChainer
  examples, w2 = wmtm-style run, w3 = benchgen KB).
- **Answers (sanity gate)**: every file-run's stdout is compared byte for byte with S0's recorded base stdout
  (evidence/s0-2026-11-24/runs/base), with exit status and failed tests (.check per run; timer runs also check the
  timer report: no stack overflow, no frame lost to a non-local exit, time inside frames at most the run).
- **Host**: pop-os, i7-1165G7 (4 cores / 8 threads), shared with other agents (load 2-4 while measuring).

## 2. Method

**Timers (Do 1).** Patch 0011 adds a build flag CETTA_EVPROF (default 0: the macro is an empty statement and
cetta_evprof.c is not built). With CETTA_EVPROF=1, 112 evaluator functions (bench/p/evprof_sites.txt, inserted by
tools/p_evprof_insert.py) open a frame on entry, closed on every return path by a cleanup attribute; malloc, calloc,
realloc and free are wrapped at link time (-Wl,--wrap) and are frames of their own (free(NULL) only counted). Time is
**exclusive**: a tick belongs to the innermost timed function, so all shares add up to the run (plus "outside":
loading and parsing). Each function carries a phase:

| phase | holds | grouped as (the order's phases) |
|---|---|---|
| eqsel | equation lookup and selection, call preparation, per-call admission/safety checks, dispatch-mode caches, opening equation cursors | equation lookup and selection |
| prepcompile | compiling prepared-pure programs at call time | equation lookup and selection |
| match | unification and matching, bindings construction and lookup | unification and matching |
| dispatch | the PeTTa search machine's goal loop and goal dispatch; the open-equation machine running bodies | dispatch |
| choice, prepexec | choice points, backtracking, answer collection; running prepared programs | dispatch |
| copy | bindings application, deep copies, frame instantiation, fresh variables | term allocation and copying |
| alloc | malloc/calloc/realloc/free of CeTTa (and the statically linked MORK bridge), arena growth, choice release | term allocation and copying |
| space | starting space queries (also how a call finds its equations in &self), equation cursors over the store, add/remove | space access |
| prolog | calls into SWI-Prolog, with term conversion both ways | Prolog |

**Overhead, measured and subtracted.** At exit the runtime times 21 x 200,000 empty child frames under an empty parent:
c_in (ticks a frame costs inside itself) and c_out (ticks it costs its parent), median of the 21. Corrected self time
= self - calls x c_in - child frames x c_out. The **residual** (corrected run time against the timers-off build's
wall time, same file and round) is reported; a second set of tables spreads it evenly over all frames
(evprof_report.py --residual) to show how much the shares depend on it.

**Sampler (cross-check).** S1's C-level sampler (0005+0006; SIGPROF every 1 ms of CPU, native stacks with inlined
frames), RUNS=3. Each sample gets the phase of the innermost timed function on its stack, as the timers charge time;
samples inside libc's allocator are "alloc" (bench/p/sample_phases.py). It also gives S1's leaf categories and, per
timed function, what it inlines.

**Counters.** CeTTa's own runtime counters (ENABLE_RUNTIME_STATS=1): cache hits and misses, revision bumps, compile
attempts (bench/p/stats_report.py).

**Zero-code calibrations.** On the timers-off build: CeTTa's reference switches CETTA_PREPARED_PURE_REFERENCE=1 (the
prepared-pure call path declines every call) and CETTA_OPEN_EQUATIONS_REFERENCE=1 (no open-equation machine); and
LD_PRELOAD of mimalloc 2.1.7 (built from source, sha256 in binaries.txt), which takes every malloc/free of the process.

**Runs.** The numbers below come from the quiet matrix (bench/p/run_matrix.sh): 3 rounds, each running every
configuration on w1-w3 (off, ev, off+0012, ev+0012, prepared path off, open-equation machine off, mimalloc), forward
order in odd rounds and reversed in even ones, with nothing else of this task running; plus a second matrix
(off vs prepared-path-off + mimalloc together, 3 rounds). Earlier runs that shared the host with builds (a first
off/ev RUNS=3, the counters, the sampler RUNS=3, an ablation pilot) are kept in evidence; their shares agree with the
quiet ones, their timings are not used.

## 3. Results

### 3.1 Answers

Every file-run in this task gave S0's stdout byte for byte, with exit 0 and 0 failed tests: the quiet matrix 273/273 file-runs (7 configurations x 39), the second matrix
78/78, the sampler runs 39/39, the earlier off/timer/counter runs 91/91 and the ablation pilot 53/53 (534 file-runs).
The 78 timer runs of the quiet matrix also pass the timer-report check (no stack overflow, no frame unwound by a
non-local exit, time inside frames at most the run).

### 3.2 Instrumentation overhead (acceptance 1)

| | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| files x runs | 11 x 3 | 1 x 3 | 1 x 3 |
| timed frames per run (M) | 330.8 | 176.9 | 625.9 |
| c_in / c_out per frame (ticks, median over files) | 20.7 / 22.3 | 19.5 / 22.3 | 22.1 / 23.8 |
| timers-on wall / timers-off wall, raw | 1.208 (1.190-1.271) | 1.335 (1.161-1.431) | 1.286 (1.244-1.335) |
| timers-on wall corrected / timers-off wall (residual) | 1.066 (1.049-1.132) | 1.201 (1.032-1.281) | 1.114 (1.087-1.157) |
| timers-on user / timers-off user | 1.211 (1.194-1.277) | 1.335 (1.162-1.433) | 1.286 (1.246-1.335) |
| frames left by non-local exits (unwound) | 0 | 0 | 0 |

The timers cost 21-34% of user time (330 / 177 / 626 million frames per run). Calibration explains half to two thirds of it
(c_in + c_out = 42-46 ticks, 15-16 ns per frame); the rest (7% / 20% / 11%) is what calibrated empty frames do not
see (cache and branch effects on real code). With that residual spread over all frames instead, the grouped shares
move by at most 2.8 points (allocation and copying down, dispatch up), so the conclusions do not depend on it:

| phase | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| equation lookup and selection | 28.6% (28.6%-28.8%) | 24.1% (24.0%-24.2%) | 23.9% (23.9%-24.0%) |
| unification and matching | 9.2% (9.1%-9.3%) | 11.2% (11.2%-11.3%) | 13.1% (12.8%-13.3%) |
| dispatch | 32.5% (32.4%-32.9%) | 34.6% (33.3%-35.2%) | 33.5% (32.9%-33.8%) |
| term allocation and copying | 18.6% (17.8%-18.8%) | 16.9% (15.5%-19.2%) | 19.0% (18.7%-19.7%) |
| space access | 4.7% (4.7%-4.8%) | 6.0% (5.8%-6.2%) | 5.1% (5.1%-5.3%) |
| Prolog (foreign calls) | 5.2% (5.1%-5.4%) | 6.7% (6.1%-7.2%) | 4.6% (4.5%-4.7%) |
| outside | 1.2% (1.2%-1.2%) | 0.4% (0.3%-0.4%) | 0.7% (0.7%-0.7%) |

### 3.3 Phase shares (acceptance 1)

Overhead-corrected shares of run time, median over 3 runs (min-max):

| phase | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| equation lookup and selection | 28.4% (28.2%-28.4%) | 23.9% (23.8%-24.0%) | 23.6% (23.5%-23.6%) |
| unification and matching | 9.2% (9.2%-9.3%) | 11.2% (11.2%-11.2%) | 13.8% (13.8%-13.8%) |
| dispatch | 32.1% (32.0%-32.2%) | 33.2% (33.1%-33.3%) | 32.5% (32.2%-32.6%) |
| term allocation and copying | 19.6% (19.5%-19.7%) | 19.7% (19.6%-19.8%) | 20.2% (20.2%-20.5%) |
| space access | 4.7% (4.6%-4.7%) | 5.7% (5.6%-5.8%) | 4.9% (4.9%-5.0%) |
| Prolog (foreign calls) | 4.9% (4.9%-5.0%) | 5.9% (5.9%-6.1%) | 4.3% (4.2%-4.3%) |
| outside | 1.1% (1.1%-1.2%) | 0.3% (0.3%-0.3%) | 0.7% (0.6%-0.7%) |

The same by phase:

| phase | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| dispatch (goal dispatch and evaluation loop (incl. open-equation machine)) | 27.6% (27.6%-27.7%) | 29.0% (28.9%-29.0%) | 28.5% (28.2%-28.5%) |
| choice (choice points, backtracking, answer collection) | 2.4% (2.4%-2.4%) | 2.6% (2.6%-2.7%) | 2.5% (2.4%-2.5%) |
| eqsel (equation lookup and selection, call preparation, admission checks) | 24.2% (24.2%-24.3%) | 20.7% (20.6%-20.8%) | 17.3% (17.3%-17.4%) |
| prepcompile (prepared-program compilation (call preparation that builds code)) | 4.1% (4.0%-4.1%) | 3.2% (3.2%-3.2%) | 6.2% (6.2%-6.3%) |
| prepexec (running prepared programs and prepared queries) | 2.0% (2.0%-2.1%) | 1.6% (1.6%-1.6%) | 1.6% (1.6%-1.6%) |
| match (unification and matching, bindings) | 9.2% (9.2%-9.3%) | 11.2% (11.2%-11.2%) | 13.8% (13.8%-13.8%) |
| copy (term copying and instantiation) | 10.6% (10.6%-10.7%) | 10.8% (10.7%-10.8%) | 11.1% (11.1%-11.2%) |
| alloc (allocation and freeing (malloc/calloc/realloc/free, arena growth)) | 8.9% (8.9%-9.1%) | 8.9% (8.9%-9.0%) | 9.1% (9.0%-9.3%) |
| space (space access (space queries, equation cursors, add/remove)) | 4.7% (4.6%-4.7%) | 5.7% (5.6%-5.8%) | 4.9% (4.9%-5.0%) |
| prolog (SWI-Prolog calls and term conversion) | 4.9% (4.9%-5.0%) | 5.9% (5.9%-6.1%) | 4.3% (4.2%-4.3%) |
| outside (outside every timed function (loading, parsing, top level)) | 1.1% (1.1%-1.2%) | 0.3% (0.3%-0.3%) | 0.7% (0.6%-0.7%) |

Corrected run time per workload (s, median over runs): w1-examples 39.0, w2-wmtm 22.9, w3-benchgen 66.6

Cross-check with the sampler (the timers' corrected shares vs the sampler's, median of 3 runs each):

| phase | w1-examples timers / sampler | w2-wmtm timers / sampler | w3-benchgen timers / sampler |
|---|---|---|---|
| dispatch | 27.6% / 26.9% | 29.0% / 28.5% | 28.5% / 27.6% |
| choice | 2.4% / 2.3% | 2.6% / 2.4% | 2.5% / 3.4% |
| eqsel | 24.2% / 23.8% | 20.7% / 20.7% | 17.3% / 17.2% |
| prepcompile | 4.1% / 3.9% | 3.2% / 3.2% | 6.2% / 5.9% |
| prepexec | 2.0% / 1.9% | 1.6% / 1.6% | 1.6% / 1.5% |
| match | 9.2% / 8.7% | 11.2% / 10.5% | 13.8% / 12.4% |
| copy | 10.6% / 10.3% | 10.8% / 10.5% | 11.1% / 10.9% |
| alloc | 8.9% / 11.2% | 8.9% / 11.1% | 9.1% / 11.5% |
| space | 4.7% / 4.6% | 5.7% / 5.5% | 4.9% / 4.7% |
| prolog | 4.9% / 5.2% | 5.9% / 5.9% | 4.3% / 4.4% |
| outside | 1.1% / 1.2% | 0.3% / 0.3% | 0.7% / 0.6% |
| samples (3 runs) | 188130 | 105066 | 315564 |

S1's native categories (innermost named function, bench/s1/sample_categories.py), same samples:

| category | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| matching, unification and fact iteration | 22.7% | 24.1% | 24.5% |
| equation selection and call preparation | 20.7% | 17.0% | 17.6% |
| goal dispatch and evaluation | 15.5% | 15.9% | 15.7% |
| allocation and freeing | 13.8% | 13.9% | 14.2% |
| copying and instantiation | 9.8% | 10.3% | 10.3% |
| hashing, interning and term universe | 9.0% | 9.8% | 9.4% |
| prolog (SWI-Prolog and the foreign boundary) | 5.1% | 5.7% | 4.4% |
| grounded ops and arithmetic | 1.4% | 1.4% | 1.7% |
| choice points, trail and backtracking | 1.3% | 1.1% | 1.4% |
| space mutation (add/remove atoms) | 0.7% | 0.9% | 0.7% |
| other machine | 0.0% | 0.0% | 0.1% |
| profiler | 0.0% | 0.0% | 0.0% |

The two instruments agree within 1.5 points on every phase except **alloc**, where the sampler sees 2-2.5 points more:
it also counts the allocations of libswipl and libc's internal ones, which the link-time wrappers do not, and its leaf
attribution of the stripped libc is approximate. S1's leaf categories (lower table) cut the same time by *kind of
operation* rather than by *evaluator stage* (e.g. bindings lookups done during dispatch count as matching there), which
is why "matching" is 23-25% there and 9-14% here.

### 3.4 Where in each phase: the functions that hold the time

Top timed functions by corrected self time (share of the workload's run):

| function | phase | w1-examples self | calls/run | w2-wmtm self | calls/run | w3-benchgen self | calls/run |
|---|---|---|---|---|---|---|---|
| petta_machine_dispatch_solve | dispatch | 4.82% | 2.97M | 4.93% | 1.67M | 5.26% | 6.11M |
| bindings_apply_fold | copy | 4.34% | 2.36M | 4.24% | 1.5M | 4.25% | 5.7M |
| oem_run_body_mode | dispatch | 3.92% | 2.63M | 3.68% | 1.33M | 3.47% | 3.74M |
| prepared_pure_program_cache_has_decline_head | eqsel | 5.24% | 5.37M | 2.74% | 2.33M | 2.08% | 6.96M |
| prepared_pure_compile_pending_heads | prepcompile | 2.50% | 0.156M | 2.19% | 0.0645M | 4.45% | 0.438M |
| bindings_builder_add_id_internal | match | 2.63% | 3.1M | 3.29% | 1.82M | 2.79% | 6.96M |
| oem_resolve_in | match | 2.27% | 7.51M | 2.86% | 4.27M | 2.61% | 13.1M |
| cetta_open_equation_cursor_open | eqsel | 2.49% | 1.31M | 2.75% | 0.646M | 2.12% | 1.94M |
| petta_machine_dispatch_goal | dispatch | 2.19% | 6.57M | 2.32% | 3.63M | 2.72% | 12.7M |
| atom_deep_copy_session_copy | copy | 2.16% | 1.93M | 2.20% | 1.25M | 2.45% | 3.55M |
| malloc | alloc | 2.02% | 37.1M | 2.14% | 21.7M | 2.24% | 64.7M |
| free | alloc | 1.92% | 66.9M | 2.14% | 36.4M | 2.22% | 114M |
| oem_cursor_run | dispatch | 1.86% | 2.1M | 1.90% | 1.06M | 2.24% | 3.26M |
| petta_machine_start_space_query | space | 2.04% | 1.77M | 2.02% | 0.89M | 1.89% | 2.51M |
| petta_machine_next_local | dispatch | 1.79% | 0.123M | 2.10% | 0.0852M | 2.01% | 0.167M |
| petta_machine_open_resume | dispatch | 1.75% | 0.772M | 1.87% | 0.407M | 1.93% | 1.27M |
| petta_machine_advance_choice | choice | 1.49% | 0.476M | 1.83% | 0.325M | 1.91% | 1.14M |
| calloc | alloc | 1.81% | 7.72M | 1.57% | 3.59M | 1.59% | 11.4M |
| petta_eval_machine_builtin_equations | eqsel | 1.56% | 11.8M | 1.60% | 6.42M | 1.81% | 21.6M |
| petta_eval_open_program_lookup_locked | eqsel | 2.06% | 3.87M | 1.65% | 1.88M | 1.09% | 5.24M |
| petta_libpl_from_term | prolog | 1.21% | 0.0529M | 1.75% | 0.0329M | 1.44% | 0.0773M |
| realloc | alloc | 1.46% | 23.3M | 1.36% | 11.8M | 1.42% | 40.5M |
| petta_machine_start_open_term_answers | dispatch | 1.46% | 1.29M | 1.44% | 0.63M | 1.27% | 1.91M |
| petta_machine_extension_named_arity_using | dispatch | 1.28% | 3.9M | 1.55% | 2.04M | 1.31% | 6.58M |
| prepared_pure_program_compile_closed_mode | prepcompile | 1.13% | 0.156M | 0.93% | 0.0645M | 1.75% | 0.438M |
| petta_libpl_registered_call | prolog | 1.24% | 0.0399M | 1.56% | 0.0255M | 0.92% | 0.0515M |
| petta_libpl_call | prolog | 1.44% | 0.0567M | 1.22% | 0.0402M | 0.89% | 0.0805M |
| petta_specializer_query_value_execution_admission | eqsel | 1.23% | 1.78M | 1.25% | 0.893M | 1.04% | 2.52M |
| petta_table_safety_scan_relation | eqsel | 1.30% | 0.76M | 1.30% | 0.375M | 0.90% | 0.907M |
| petta_libpl_to_term | prolog | 1.04% | 3.04M | 1.43% | 2.29M | 1.01% | 5.73M |

Per-call cost (w2, round 1, corrected self time / calls):

| function | share of w2 | calls | ns per call |
|---|---|---|---|
| petta_eval_machine_builtin_equations | 1.60% | 6.42M | 57 |
| petta_eval_relation_admission_entry | 1.11% | 1.34M | 191 |
| petta_eval_open_program_lookup_locked | 1.65% | 1.88M | 200 |
| petta_table_safety_scan_relation | 1.30% | 0.37M | 794 |
| petta_specializer_query_value_execution_admission | 1.25% | 0.89M | 321 |
| petta_program_candidate_snapshot_lease_profiled | 1.10% | 1.26M | 199 |
| petta_program_refresh_callability | 0.75% | 1.26M | 136 |
| petta_machine_extension_named_arity_using | 1.55% | 2.04M | 174 |
| prepared_pure_program_cache_has_decline_head | 2.74% | 2.33M | 269 |
| prepared_pure_compile_pending_heads | 2.19% | 0.06M | 7,774 |
| cetta_open_equation_cursor_open | 2.77% | 0.65M | 981 |
| bindings_apply_fold | 4.24% | 1.50M | 648 |
| atom_deep_copy_session_copy | 2.18% | 1.25M | 399 |
| malloc / free / calloc / realloc | 2.15 / 2.18 / 1.57 / 1.36% | 21.7 / 36.4 / 3.6 / 11.8M | 23 / 14 / 100 / 26 |
| petta_machine_dispatch_solve | 4.93% | 1.67M | 677 |
| oem_run_body_mode | 3.68% | 1.33M | 635 |
| bindings_builder_add_id_internal | 3.29% | 1.82M | 414 |

### 3.5 Counters (CeTTa's own, one run each; summed over each workload's files)

| counter | w1 | w2 | w3 |
|---|---|---|---|
| space-equation-revision-bump (equation-relevant space revisions) | 32,770 | 16,540 | 28,237 |
| prepared-pure-call-admission / commit | 384,577 / 259,116 | 176,813 / 134,622 | 453,124 / 343,993 |
| prepared-pure-call-decline | 795,238 | 241,421 | 647,248 |
| prepared-pure-call-compile-attempt / compile-decline | 83,548 / 40,952 | 39,681 / 17,848 | 268,196 / 154,630 |
| prepared-pure-call-program-cache hit / store | 434,118 / 53,320 | 184,026 / 26,882 | 376,238 / 132,904 |
| prepared-pure-call-negative-cache hit / store | 1,732,298 / 139,129 | 639,591 / 70,463 | 2,755,313 / 478,790 |
| petta-specializer-callable-cache hit / miss | 7,207,586 / 115,548 | 3,771,394 / 22,294 | 8,773,669 / 24,755 |
| petta-named-arity-source-cache hit / miss | 1,890,770 / 245,291 | 1,052,629 / 128,383 | 4,113,693 / 505,513 |
| petta-relation-safety-cache-miss | 238,549 | 104,568 | 405,620 |
| petta-activation-admission-cache hit / miss | 44,514 / 22,625 | 32,847 / 19,408 | 32,225 / 42,659 |
| arena-spare-miss (arena blocks taken from malloc) | 6,585,382 | 3,603,165 | 10,677,037 |
| bindings-apply | 9,487,991 | 5,250,751 | 17,510,920 |
| term-universe-copy call / memo hit | 527,989 / 524,535 | 249,912 / 216,323 | 850,055 / 765,489 |
| scratch-arena bytes / query-episode survivor bytes | 1.94 / 2.52 GB | 1.25 / 1.53 GB | 3.61 / 5.55 GB |

Full list: evidence/p-2026-11-27/stats.json.

### 3.6 Zero-code calibrations and the prototype (quiet matrix, user time vs the timers-off build, paired by round)

| configuration | answers (file-runs same as S0) | w1-examples user s (median) / ratio to off, median (min-max) | w2-wmtm user s (median) / ratio to off, median (min-max) | w3-benchgen user s (median) / ratio to off, median (min-max) |
|---|---|---|---|---|
| abloe | 39/39 | 42.3 / 1.144 (1.129-1.290) | 21.4 / 1.081 (1.028-1.176) | 65.4 / 1.138 (1.081-1.149) |
| ablpp | 39/39 | 32.4 / 0.897 (0.845-0.945) | 18.2 / 0.918 (0.887-1.026) | 51.1 / 0.893 (0.872-0.895) |
| ev | 39/39 | 44.6 / 1.229 (1.213-1.296) | 25.4 / 1.338 (1.165-1.436) | 76.5 / 1.287 (1.247-1.336) |
| evproto | 39/39 | 42.2 / 1.148 (1.146-1.193) | 23.6 / 1.170 (1.164-1.342) | 69.7 / 1.217 (1.212-1.268) |
| mimalloc | 39/39 | 33.6 / 0.915 (0.876-0.940) | 18.2 / 0.903 (0.900-0.986) | 52.9 / 0.923 (0.873-0.939) |
| off | 39/39 | 36.7 / 1.000 (1.000-1.000) | 19.8 / 1.000 (1.000-1.000) | 57.3 / 1.000 (1.000-1.000) |
| proto | 39/39 | 37.6 / 1.024 (0.936-1.037) | 21.3 / 1.075 (0.981-1.132) | 62.1 / 1.033 (1.001-1.085) |

- **Prepared-pure call path off: 0.90 / 0.92 / 0.89**, in every round below the timers-off build (w3: 50.0-54.2 s vs
  57.2-60.5 s); answers identical. On these workloads the fast path costs more than it gains.
- **Open-equation machine off: 1.14 / 1.08 / 1.14**: the open-equation machine pays for itself.
- **mimalloc: 0.92 / 0.90 / 0.92**, in every round below the base; answers identical.
- **Prepared path off and mimalloc together** (second matrix, 3 rounds, off vs both):

| configuration | answers (file-runs same as S0) | w1-examples user s (median) / ratio to off, median (min-max) | w2-wmtm user s (median) / ratio to off, median (min-max) | w3-benchgen user s (median) / ratio to off, median (min-max) |
|---|---|---|---|---|
| off | 39/39 | 37.3 / 1.000 (1.000-1.000) | 19.0 / 1.000 (1.000-1.000) | 57.8 / 1.000 (1.000-1.000) |
| ppmi | 39/39 | 29.3 / 0.783 (0.776-0.891) | 16.4 / 0.864 (0.808-0.882) | 47.5 / 0.820 (0.791-0.822) |

  Together **0.78 / 0.86 / 0.82** (1.28x / 1.16x / 1.22x) with no code change and identical answers, against the sum
  of the two alone (18.8% / 17.9% / 18.4% saved): close to additive on w1 and w3, less on w2.
- **Prototype 0012** (off+0012 vs off): 1.02 / 1.08 / 1.03, with ranges 0.94-1.04 / 0.98-1.13 / 1.00-1.09: inside this
  host's spread (the base alone moves 57.2-60.5 s on w3 across rounds); the effect it was built to calibrate is a few
  percent and is measured per function instead:

**w1-examples** (runs 1,2,3; call counts identical outside the allocator: NO): corrected total 38.99 s -> 36.65 s (-6.0%)

| function | without 0012 (s) | with 0012 (s) | change |
|---|---|---|---|
| prepared_pure_program_cache_has_decline_head | 2.056 | 0.246 | -1.811 (-88%) |
| prepared_pure_closed_invocation_try | 0.316 | 0.236 | -0.080 (-25%) |
| petta_relation_specialization_relevance | 0.282 | 0.239 | -0.044 (-15%) |
| petta_eval_machine_prepared_refused | 0.085 | 0.046 | -0.040 (-47%) |
| petta_machine_dispatch_goal | 0.854 | 0.818 | -0.036 (-4%) |
| petta_eval_machine_open_answer_cursor | 0.107 | 0.072 | -0.035 (-33%) |
| space_equation_cursor_member_next | 0.362 | 0.326 | -0.035 (-10%) |
| calloc | 0.705 | 0.675 | -0.030 (-4%) |

**w2-wmtm** (runs 1,2,3; call counts identical outside the allocator: NO): corrected total 22.88 s -> 20.82 s (-9.0%)

| function | without 0012 (s) | with 0012 (s) | change |
|---|---|---|---|
| prepared_pure_program_cache_has_decline_head | 0.628 | 0.106 | -0.522 (-83%) |
| oem_run_body_mode | 0.843 | 0.778 | -0.065 (-8%) |
| free | 0.499 | 0.435 | -0.063 (-13%) |
| petta_machine_dispatch_solve | 1.127 | 1.071 | -0.056 (-5%) |
| cetta_open_equation_cursor_open | 0.634 | 0.587 | -0.047 (-7%) |
| bindings_builder_add_id_internal | 0.753 | 0.706 | -0.047 (-6%) |
| bindings_apply_fold | 0.970 | 0.924 | -0.046 (-5%) |
| malloc | 0.492 | 0.448 | -0.044 (-9%) |

**w3-benchgen** (runs 1,2,3; call counts identical outside the allocator: NO): corrected total 66.65 s -> 60.37 s (-9.4%)

| function | without 0012 (s) | with 0012 (s) | change |
|---|---|---|---|
| prepared_pure_program_cache_has_decline_head | 1.365 | 0.309 | -1.055 (-77%) |
| petta_machine_dispatch_solve | 3.519 | 3.200 | -0.319 (-9%) |
| prepared_pure_compile_pending_heads | 2.969 | 2.743 | -0.226 (-8%) |
| petta_machine_dispatch_goal | 1.838 | 1.645 | -0.193 (-11%) |
| bindings_apply_fold | 2.831 | 2.641 | -0.190 (-7%) |
| oem_run_body_mode | 2.312 | 2.142 | -0.170 (-7%) |
| petta_semantics_match_cons_constraint_mode | 1.362 | 1.204 | -0.158 (-12%) |
| oem_cursor_run | 1.492 | 1.339 | -0.154 (-10%) |

The function the prototype targets, the refusal check that walked the cache's list, loses 77-88% of its time
(2.06 -> 0.25 s on w1, 0.63 -> 0.11 s on w2, 1.37 -> 0.31 s on w3); over the four lookup functions of C2 the measured
fraction is 0.73 / 0.63 / 0.54. The call counts of the evaluator's functions differ between the two builds only in
functions whose counts also differ between two runs of the same binary (by similar amounts: CeTTa's own run-to-run
variation, e.g. petta_relation_specialization_relevance 38,855 vs 28,339 in two runs of one build), so the prototype
changes no decision; answers are identical in all 78 file-runs. Under the timers the prototype's runs are 6-9% shorter
overall, with small gains spread over many unrelated functions (dispatch_solve -9% on w3), which looks like less cache
pollution from the list walk; without the timers that does not show above the noise, so it is not counted.

## 4. Reading the hot phases

**Equation lookup, selection and call preparation (24-28%).** A function call in the PeTTa machine
(petta_machine_try_ready_relation_invocation) is a space query on &self. Before the query runs, the call passes about
ten separate decisions, each with its own cache: builtin-equations mode, relation admission, the open-equation program
cache (a linear scan of its entries, then a 10-field match), relation table safety (a scan of candidate equations on a
miss), specializer value-execution admission, candidate snapshot leases, callability refresh, named-arity lookups
(which reach SWI's current_predicate/1 enumeration when the Prolog revision moved: 0.6-0.7% of w3 inside SWI), host
classification, and the prepared-pure path's refusal checks. Each cache re-reads and compares its own copy of the same
inputs on every call (library admission revision, symbol-table instance, active mask, profile, language, the Prolog
read token, the space program token): 50-800 ns per decision where a validated hit could cost a few ns. Many of the
caches are keyed on the **whole space's** program token, and equation-relevant space revisions happen 16,540-32,770
times per run (PeTTaChainer adds compiled rules while it runs), so one new rule invalidates every head's cached
decision.
**Prepared-pure path**: for each call CeTTa first tries to run a compiled "prepared" program. Its cache is one linked
list that the refusal check walks up to four times per call (has_decline_head: 2.1-5.2% alone); the whole cache is
cleared whenever &self's program token changes, so programs are recompiled after unrelated changes (compile attempts
39,681-268,196 per run; compile_pending_heads costs 7.8 us per call, 2.2-4.5%); and 241K-795K calls per run are
declined after paying for the attempt.

**Dispatch (32-33%).** The goal loop's time is spread over many functions, each 1-5% with nothing dominant inside
(the sampler's view of petta_machine_dispatch_solve's 5%: 1% its own code, the rest a dozen inlined helpers at
0.05-0.2% each). No single local change reaches it; it is the per-goal interpretation cost that compiling the
chainer's agenda functions (S1/S2) would remove.

**Term allocation and copying (20%).** Allocation: 37-65 million mallocs and 67-114 million non-NULL frees per run on
w1/w3 from CeTTa alone, plus 23-41 million reallocs; the biggest callers (sampler) are answer-choice release, equation
cursor open/close (each call callocs a cursor struct, two arenas, frame and query arrays and frees them at close), arena
growth (arena-spare-miss 3.6-10.7 million) and the bindings builder. Copying: bindings application rebuilds terms from
(skeleton, bindings) pairs (bindings_apply_fold 4.2-4.3%, 650 ns per call; 5.3-17.5 million applications per run),
deep-copy sessions copy answers between arenas (2.2-2.5%; 6.6-24 million recorded nodes), and term-universe copies hit
their memo 87-99% of the time, i.e. mostly copy terms they have copied before.

**Unification and matching (9-14%).** Bindings builder insertion (2.6-3.3%), open-equation-machine resolution
(2.3-2.9%), the decoded-atoms worklist matcher and cons-constraint matching; diffuse, 0.1-0.4 us per call.

**Space access (5-6%)** and **Prolog (4-6%)** are each too small for a change aimed at them to reach the target.

## 5. Ranked changes (acceptance 2)

Estimates are Amdahl over the measured shares: saved = touched share x fraction removed; the changes touch disjoint
functions, so savings add. "touched" is the corrected self time of the functions each change alters (bench/p/estimates.py
lists them); "measured" means a run measured the saving, "assumed" that the fraction is stated, not measured.

| change | group | basis | fraction removed | w1-examples touched / saved | w2-wmtm touched / saved | w3-benchgen touched / saved |
|---|---|---|---|---|---|---|
| C1 | per-call admission, safety and dispatch-mode checks (validation of 5-10 revision fields, table-safety scans, specializer admission, arity lookups through Prolog) | assumed | 0.60 | 17.2% / 10.3% | 16.4% / 9.8% | 14.4% / 8.7% |
| **C1 total** | Call-preparation decisions cached per call site and invalidated per head | | | 17.2% / **10.3% (1.12x)** | 16.4% / **9.8% (1.11x)** | 14.4% / **8.7% (1.09x)** |
| C2 | the prepared-pure call path: refusal checks and cache walks, compile attempts, prepared execution (saving measured end to end with the path off, CeTTa's own reference switch; an adaptive policy does at least as well where its bookkeeping is cheap) | measured | measured | 12.1% / 10.3% (f=0.85) | 8.1% / 8.2% (f=1.01) | 10.5% / 10.7% (f=1.02) |
| **C2 total** | Prepared-call fast path admitted only where it pays (per-head), with an indexed cache (0012) and programs kept across unrelated program changes | | | 12.1% / **10.3% (1.11x)** | 8.1% / **8.2% (1.09x)** | 10.5% / **10.7% (1.12x)** |
| C3 | malloc/calloc/realloc/free and arena growth (saving measured end to end by the mimalloc swap, which also speeds SWI-Prolog's and libc's own allocations) | measured | measured | 8.9% / 8.5% (f=0.95) | 8.9% / 9.7% (f=1.08) | 9.1% / 7.7% (f=0.85) |
| **C3 total** | Allocation: a faster general allocator (mimalloc), then pools for per-call machine records | | | 8.9% / **8.5% (1.09x)** | 8.9% / **9.7% (1.11x)** | 9.1% / **7.7% (1.08x)** |
| C4 | bindings application, deep copies, frame instantiation | assumed | 0.35 | 10.6% / 3.7% | 10.8% / 3.8% | 11.1% / 3.9% |
| **C4 total** | Copying: share ground and settled subterms instead of re-instantiating them | | | 10.6% / **3.7% (1.04x)** | 10.8% / **3.8% (1.04x)** | 11.1% / **3.9% (1.04x)** |
| **all four** | | | | **32.8% (1.49x)** | **31.4% (1.46x)** | **30.9% (1.45x)** |

**C2. Prepared-call fast path admitted only where it pays** (rank 1 by measured gain).
*What changes:* (a) the cache is indexed by call key (prototype 0012, ~150 lines: refusal checks -77..-88%); (b) a
per-head admission policy (stop trying heads whose prepared attempts decline or do not beat the reference path, with
periodic retries), instead of trying every call; (c) the cache invalidated per head instead of cleared on any change to
&self. *Estimate:* **8-11% (measured)** as the floor: the crudest policy, "never", already gives 0.90/0.92/0.89 with
identical answers; a per-head policy keeps the path where it wins. *Risk to answers:* low (the reference path is the
semantics; the switch is CeTTa's own and answers were identical in 39/39 file-runs; the index changes no decision).
*Size:* small to medium (0012 is ~150 lines; the policy a few hundred lines in eval.c).

**C1. Call-preparation decisions cached per call site, invalidated per head** (rank 2).
*What changes:* one dispatch epoch for the inputs every cache re-reads (library admission revision, symbol table,
mask, profile, language, Prolog read token), bumped when any of them changes; per-(head, arity) equation versions
instead of the whole space's program token, so a new rule for one head does not invalidate the others; and a per-call-
site inline cache in the plan node that holds the whole routing decision (builtin mode, admission, open-equation
program, table safety, specializer admission, named arity) and validates with one compare. *Estimate:* touched 14-17%;
**fraction 0.6 assumed** -> 8.7-10.3%. Basis: the decisions cost 50-800 ns per call where a hit costs a few ns, the
caches that count hits hit 88-100% (specializer callable 98-100%, named arity 88-89%), the prototype removed 54-73% of
the time of the C2 lookups it indexed (measured), and 40% is kept for real misses and the work these functions do
beyond the decision. Sensitivity: fraction 0.4 / 0.8 gives 5.8-6.9% / 11.5-13.8%. *Risk:* medium: a missed
invalidation input changes answers; CeTTa's existing CETTA_PETTA_DISPATCH_MEMO=check pattern (compute both, abort on a
mismatch) should guard the new cache in tests. *Size:* large (eval.c, petta_program.c, petta_specializer.c,
petta_libpl.c; ~1-2 thousand lines).

**C3. A faster allocator, then pools for per-call machine records** (rank 3).
*What changes:* link mimalloc (or jemalloc) as CeTTa's allocator; then per-machine free lists for fixed-size records
(equation cursors and their arrays, answer choices, frame schemas) and reuse of a cursor's arenas instead of
free/calloc per call. *Estimate:* **7.7-9.7% (measured** with mimalloc via LD_PRELOAD, every round below the base);
pools would add to it (not estimated). *Risk:* low (allocation only; answers identical in 39/39 file-runs). *Size:*
small for the allocator (a build dependency, a decision for Zar); medium for pools.

**C4. Copying: share ground and settled subterms instead of re-instantiating them** (rank 4).
*What changes:* return structure-shared views (skeleton + bindings, or the original atom) when a bindings application
or an answer transfer finds no variable to replace in a subterm, and reuse the copy of a term already copied into the
destination arena (the term-universe memo already finds 87-99% of copies are repeats). *Estimate:* touched 10.6-11.1%;
**fraction 0.35 assumed** -> 3.7-3.9%. *Risk:* medium (aliasing: a shared subterm must not be mutated or freed with
its arena). *Size:* medium to large (match.c, atom.c, term_universe.c).

Not ranked: matching (9-14%, diffuse) and dispatch (32-33%, diffuse); no general local change was found that reaches
a meaningful fraction of either.

## 6. Go/no-go (acceptance 3)

**1.5x: plausible but borderline. 2x: no.**

- The four changes together: **32.9% / 31.4% / 30.9% saved -> 1.49x / 1.46x / 1.45x** (with the overhead residual
  spread over frames: 32.9 / 31.6 / 31.1%). Half of it is measured: C2's floor and C3 add to 18.8% / 17.9% / 18.4% on their
  own and were measured together at 0.78 / 0.86 / 0.82 of the base's user time (1.28x / 1.16x / 1.22x). The other half rests on C1's assumed fraction (0.6) and C4's (0.35).
  With the joint measurement for C2+C3 in place of their sum: 35.7% / 27.2% / 30.6% -> **1.56x / 1.37x / 1.44x**.
  1.5x needs 33.3% saved: reached on w1, close on w3, not on w2 under the stated assumptions; it depends on C1 doing at
  least as well as assumed (C1 at 0.8 instead of 0.6: 1.51-1.57x from the sum; at 0.4: 1.39-1.42x).
- **2x is not plausible from evaluator-internal changes.** 2x needs 50% saved. The four changes touch 49% / 44% / 45% of
  the run in total; removing *all* of it would give 1.95x / 1.79x / 1.82x, and no change removes all of what it touches.
  The rest is mostly the dispatch loop and matching (41-46% together), spread thin across many functions: only compiling the
  agenda functions themselves (the S1/S2 direction) removes per-goal interpretation at that scale.
- **Recommendation:** go for a fork experiment of C3 and C2 first (measured, low risk, small), then C1 if the goal is
  ~1.4-1.5x; do not expect 2x from this route.

## 7. Limits

- One host, shared with other agents: whole-run timings vary 5-9% between rounds (w3 base 57.2-60.5 s), so whole-run
  effects below ~5% are not resolved; the prototype's end-to-end effect is in that band and is reported per function.
- The timers' residual overhead (7-20% after calibration) is not attributed exactly; the residual-spread tables bound
  its effect on the grouped shares (<= 2.8 points). Very short functions (malloc/free: 14-26 ns per call) carry the most
  relative overhead; the sampler's alloc share (11-11.5%) is the other bound.
- The allocator figure (C3) is a process-wide swap (also SWI-Prolog's allocations); pooling inside CeTTa is not
  measured. C2's figure is the path switched off globally; an adaptive policy is estimated to do at least as well, not
  measured. C1 and C4 fractions are assumptions with the stated basis.
- Savings are added as if independent; C2+C3 measured together (0.78 / 0.86 / 0.82 of the base's user time (1.28x / 1.16x / 1.22x)) vs their sum (18.8% / 17.9% / 18.4%) shows the size of
  the interaction for that pair.
- CeTTa's run-to-run call counts vary in a few functions (same binary, two runs); answers do not.
- Workloads are S0's w1-w3 only (synthetic data); PeTTaChainer b0e24f9b, not the order's original pin (see S0).

## 8. Reproduce

```
export GMP_PREFIX=<libgmp-dev root>/usr SWIPL_PKG_CONFIG_PATH=<swipl 10.0.2 root>/share/pkgconfig CETTA_SRC=<CeTTa checkout>
for v in off ev stats samp; do bench/p/build_p_cetta.sh $v; done
P_SUFFIX=-proto P_EXTRA_PATCHES=$PWD/upstream/cetta-p-proto/0012-*.patch bench/p/build_p_cetta.sh off   # and ev
RUNS=3 MIMALLOC=<libmimalloc.so> bench/p/run_matrix.sh evidence/p-2026-11-27/matrix          # quiet matrix
RUNS=3 MIMALLOC=<libmimalloc.so> bench/p/run_matrix.sh evidence/p-2026-11-27/matrix2 off ppmi
RUNS=3 bench/p/run_all.sh evidence/p-2026-11-27/samp-runs samp && bench/p/sample_report.sh evidence/p-2026-11-27/samp-runs/samp evidence/p-2026-11-27/sampler
RUNS=1 bench/p/run_all.sh evidence/p-2026-11-27/runs-contended stats
bash bench/p/recompute_p.sh                                                                     # every table
```

Evidence: evidence/p-2026-11-27/ (runs, reports, counters, sampler summaries); raw sampler stacks and a copy of
everything in /srv/hive-evidence/t-2026-11-27-cetta-internals-p-profile/ (SHA256SUMS).