# Whole-function specialisation on CeTTa's own matcher

Task t-2026-11-18-cettapack-fnspec (andy, Work msg 773). Branch hive/t-2026-11-18-cettapack-fnspec, worktree
wt/fnspec, base hive/t-2026-11-18-cettapack-next-step-probe cff658d (cettapack main 64fa80d does not hold the probe
branch). CeTTa a9b3a811 + upstream/cetta 0001-0003, unchanged, as files. Plan: docs/FNSPEC_PLAN.md (committed first,
dad6586); survey: docs/FNSPEC_SURVEY.md (9e91b46, before any compiler code). Evidence: evidence/fnspec-2026-11-18/.

**Question.** Can cettapack specialise the MeTTa function shapes that real code uses most into compiled code on
CeTTa's own matcher, with the same answers bag by bag, guarded, and how much faster is each shape than CeTTa
running it?

**Answer.** Yes, for the three shapes the survey ranks first: the compiled functions give CeTTa's answers bag by
bag on every site and test space (675 bag-test checks, 0 failures), take the residual whenever a guard fails (forced
INVALID, a second or changed equation of the function, a shadowed builtin, a type declaration, values outside the
compiled kinds, answers not in normal form), and are faster than CeTTa evaluating the same function on 18 of 19
benchmark sites (7/7 paired runs each) and as fast on the 19th:

| shape (survey rank) | fn-spec vs CeTTa evaluating the function (fn-generic), paired median | fn-spec vs the function with its match hosted (fn-hosted) | real-code site |
|---|---|---|---|
| collapse + data template (1) | **2.4x-4.0x** on the probe sites | 2.8x-4.3x | PeTTaChainer live-hypotheses: **3.0x** vs CeTTa, 3.5x vs hosted |
| stream + data template (2) | **1.9x-2.8x** on the probe sites | 2.2x-3.4x | CeTTa metagol mil:run (about 1 answer per call): **1.00x** vs CeTTa (3/7 runs faster), 1.7x vs hosted |
| collapse + filter (3) | **9.3x-18.8x** on the probe sites | 9.9x-19.1x | PeTTaChainer base-rate-cache-mode-rows: **3.5x** vs CeTTa, 4.9x vs hosted |
| stream + filter (the probe's shape, rank 8) | 7.6x-13.7x | 8.5x-15.1x (probes: 8.8x-15.7x) | - |

The gain follows what the specialised code removes: a filter costs CeTTa an if, an == and an (empty) per row, and the
compiled condition removes them (10-19x); a data template costs CeTTa only the evaluation of the instantiated
template and the answer hand-back (2-4x); a match with one or two answers per call costs CeTTa little beyond the
call itself, and compiling it gains nothing over CeTTa (mil:run, 1.00x), only over the hosted callable. Guards are
inside every fn-spec time. Peak RSS: fn-spec within 0.1 MiB of fn-hosted or below it on every site (no index).

## What was built

**Shapes (from the survey's counts, docs/FNSPEC_SURVEY.md).** Of 495 match occurrences in 11,514 equations
(CeTTa lib+examples, PeTTa lib, PeTTaChainer, wmtm), the three most frequent families the probe's fragment can grow
into are: collapse + data template (84), stream + data template (60), collapse + filter (34). The probe's own shape,
stream + filter, is rank 8 (7) and stays compiled for the four probe sites. Arithmetic on bound values occurs 0 times
as a template, so no chosen shape is arithmetic; numeric comparisons in a filter are supported and tested at edge
values.

**Compiler, tools/fnspecc.py (definition -> C).** Reads one equation (= (f $a1 ... $an) BODY) and accepts BODY only as

    stream-data      (match S P T)
    collapse-data    (collapse (match S P T))
    collapse-filter  (collapse (match S P (if C T (empty))))      either branch may be the (empty) one
    stream-filter    (match S P (if C (empty) T))

with S an argument or a named space token (&name), P one pattern or a conjunction (, P1 ... Pn) (in stream shapes a
nested (match S P2 T2) template on the same S is flattened into the conjunction), T data (no builtin, no function of
the file), and C a tree of and/or/not over ==, =alpha, <, >, <=, >= on bound variables, arguments and ground
constants. Anything else is refused with a reason (the function stays CeTTa's). Output per site: build/fnspec/<site>.c
with the plan (the texts the runtime parses once, the shape, the builtins the body relies on with their arities) and
the generated row function: the condition as C over the bound values (every comparison computed, each may abort,
before the and/or), the template by bindings_apply, the normal-form check. Example: build/fnspec/rs_base_rate_rows.c
compiles PeTTaChainer's (and (=alpha $stored $pattern) (or (== $emode any) (== $emode $mode))) into three
fs_compare calls and one C boolean expression.

**Runtime, src/cpk_fnspec.c.** The call (an embedding callback cpk-fnspec:<f>, as the stop-lines require) runs CeTTa's
conjunction matcher (space_query_conjunction) on the live space with the arguments as seed bindings, the generated
row function per row, the shape's wrapper (stream: one answer per row; collapse: one tuple in the matcher's row
order) and CeTTa's Empty convention for stream-filter (every row filtered -> one Empty per row). No PathMap, no index.
The residual is CeTTa evaluating (f args...) in the same engine.

**Guards** (docs/FNSPEC_PLAN.md B3-B6, the note's 8.2-8.4, 14, A.3), in the order a call meets them:

| guard | checked | how | failing -> |
|---|---|---|---|
| forced INVALID (test mode, fn-fallback) | every call | flag | residual |
| definition of f is the compiled one | **per &self revision**: on any call where CeTTa's program token of &self (instance id, equation revision, declaration revision, dependency epoch; space_program_token_is_current) has moved since the last check | re-read every equation of f from &self (exactly one, alpha-equal to the compiled equation, variables by bijection) and every type declaration of f (none) | INVALID until the token moves again; residual |
| builtin meanings | per &self revision, with the above | the equations and declarations in &self that could rewrite each builtin the body uses ((= (op $x1..$xk) $r) at its arity, (: op $t)), as canonical text, must equal a fresh engine's; no catch-all equation (space_equations_may_match_unnamed_head) | INVALID; residual |
| arguments | every call | the space argument is a space; the others ground and in normal form | residual |
| value kinds | every row | each comparison checks its operands: values (normal form), ground; == and =alpha on numbers only int-int or float-float (no NaN), otherwise structural equality on plain ground atoms; <, >, <=, >= only int-int or float-float | the call aborts before any answer is handed back; residual |
| answers in normal form | every answer | no variable; no head that is a grounded operation or a builtin symbol or a variable; no symbol for which space_equations_may_match_known_head(&self) holds; numbers, strings, bools fine | abort; residual |

All guard costs are inside the fn-spec timings below (the token check and argument checks per call, the kind and
normal-form checks per row). A revalidation (a few MeTTa queries of &self) happens only when &self's program changes;
in the benchmark it happens once, in the untimed warm-up call.

## Bag tests (make fnspec-difftest, part of make test)

Per site, each mode in its own engine against CeTTa evaluating the same function (fn-generic):
fn-hosted, fn-spec, fn-fallback; on the probes' six small spaces (duplicates, co-reference, equal values in distinct
variables, dangling joins, a 70-byte interned symbol, every row of a seed filtered) or the real sites' own small
spaces (duplicates, co-reference, expressions, strings and numbers in rows, absent keys), numeric edge spaces
(INT64_MAX, INT64_MIN, -0.0/0.0, mixed int/float, bigint, symbols, strings and True where numbers are compared), then
the site's benchmark KB with its benchmark queries. Path checks: every fn-spec call specialised on ordinary spaces
(on "mixed" spaces some calls take the residual by design: variables in rows, mixed kinds, non-ground arguments);
every fn-fallback call on the residual; the hosted callable called once per query.

On the first small space of each site, two sequences in a reference engine and a fn-spec engine, the same change to
&self in both, every query compared after each step, with the expected route:

| step | change to &self | expected fn-spec route |
|---|---|---|
| baseline | - | all specialised |
| second-equation-added | (= (f $a0 ...) cpk-extra) | all residual (two equations) |
| second-equation-removed | remove it | all specialised (revalidated) |
| redefined | remove f's equation, add the same body under (cpk-redefined ...) | all residual (equation differs) |
| restored | the original back | all specialised |
| builtin-shadowed | (= (<last builtin the body uses> $s0 ...) cpk-shadow) | all residual |
| builtin-unshadowed | remove it | all specialised |
| declaration-added | (: f (-> Atom Atom)) | all residual |
| declaration-removed | remove it | all specialised |
| row-not-normal | an equation that rewrites some answers (e.g. (= (g1 $t) (bumped $t))) | the affected calls residual, the rest specialised |
| recursion depth 1/10/100/1000 | (= (cpk-depth $n args) (if (== $n 0) (CALL args) (cpk-depth (- $n 1) args))), CALL f or cpk-fnspec:f | all specialised |

Results (evidence/fnspec-2026-11-18/difftest/, one file per site; make test re-runs them,
evidence/fnspec-2026-11-18/make-test.log): **21 sites, 675 checks, 0 failures**: 381 mode-against-reference checks
(fn-hosted, fn-spec, fn-fallback on every space of every site, including the benchmark KBs: e.g. tf_stream_data
173,693 answers over 40 queries, bag-equal in all modes), 210 mutation steps (10 per site), 84 recursion checks
(depths 1-1000). In every mutation sequence the guard took the residual exactly on the steps that change the
function, a builtin or the function's declaration, revalidated and specialised again when the change was undone, and
on "row-not-normal" sent only the affected calls to the residual (e.g. te sites 4 of 17, base-rate 8 of 54). On the
"mixed" spaces the calls that met a variable in a row, a mixed int/float or bigint comparison, a symbol compared with
<, or a non-ground argument took the residual (e.g. num sites 8/8, base-rate 5/5, live-hypotheses 2/4, mil:run 2/5)
and returned CeTTa's answers. The forced-INVALID guard (fn-fallback) took the residual on every call of every space.

Known-answer canary through the production path: the probe's shape on the probe sites gives the probes' recorded
answers per batch, 1,620 / 7,268 / 173,693 / 9,643 (docs/NEXT_STEP_PROBES.md), in the bag tests and in every benchmark
run. The probes' own difftest (build/cettapack-probe-<site>, all probe modes, four sites) still passes: earlier sites'
answers unchanged (make probe-difftest, in make test).

## Benchmark

Method as the probes (docs/NEXT_STEP_PROBES.md): pop-os (i7-1165G7, 8 threads, 15 GiB, shared), CeTTa a9b3a811 +
0001-0003, BUILD=mork; RUNS=7 processes per mode, REPEAT=5 batches of all queries per process, the process's median
batch, modes interleaved run by run (bench/run_fnspec.sh); every query atom built before the timed loop; one
untimed warm-up call; paired ratio = run i of the reference over run i of fn-spec (> 1: fn-spec faster). Probe sites:
the M1 KB (bench/gen_kb.py --seed 20261106, 128,576 atoms, sha256 8097f2f4...5439f5c) with the probes' seeds; real-code
sites: bench/gen_fnspec_kb.py --seed 20261118 (inputs.txt has the hashes). Modes:

- fn-generic: CeTTa evaluates the function;
- fn-hosted: the same function with its match replaced by a callable on CeTTa's conjunction matcher (cpk-hosted:f,
  this milestone's own hosted callable: the probes' hosted-generic without the pack; the rest of the body evaluated
  by CeTTa);
- fn-spec: the compiled function, guards included;
- fn-fallback: the compiled function's callable with its guard forced INVALID: the residual.

Structural checks (bench/check_fnspec_site.py, run after each site; evidence/fnspec-2026-11-18/bench/checks.txt): the
same answers per batch (and collapse items) in every mode and run; no Error or unevaluated call among the answers;
fn-spec specialised every timed call; fn-fallback took the residual on every timed call. Known-answer canary: the
probe's shape on the probe sites must give the probes' recorded answers per batch (1,620 / 7,268 / 173,693 / 9,643).

Per shape, one row per site (bench/summarize_fnspec.py on evidence/fnspec-2026-11-18/bench/runs-*.jsonl; the same file as
evidence/fnspec-2026-11-18/bench/summary.md, which make fnspec-recompute regenerates and compares):

### collapse-data (survey rank 1)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_data | 200 | 183.3 | 228.7 (215.3-243.2) | 55.9 (51.0-62.1) | 214.8 | **4.11x** (3.69-4.55x, 7/7) | 3.29x (2.90-3.80x, 7/7) | 200 tuples, 7,377 items | 44.5 / 44.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_live_hypotheses | 200 | 611.7 | 705.0 (579.2-812.0) | 203.9 (179.7-219.8) | 692.7 | **3.48x** (2.79-4.15x, 7/7) | 3.00x (2.44-3.56x, 7/7) | 200 tuples, 12,505 items | 258.7 / 258.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_data | 200 | 66.6 | 88.5 (78.5-93.2) | 30.7 (28.0-42.2) | 89.5 | **2.80x** (2.21-2.95x, 7/7) | 2.38x (1.39-3.67x, 7/7) | 200 tuples, 1,653 items | 44.3 / 44.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_data | 40 | 23,270.8 | 26,504.4 (23,859.9-27,924.5) | 6,196.2 (5,233.2-8,375.1) | 24,717.8 | **4.28x** (3.16-5.34x, 7/7) | 3.99x (2.60-4.45x, 7/7) | 40 tuples, 173,693 items | 46.3 / 52.6 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_data | 200 | 292.4 | 357.9 (314.8-534.6) | 77.6 (66.3-119.5) | 319.0 | **4.26x** (3.03-7.16x, 7/7) | 3.75x (2.58-5.12x, 7/7) | 200 tuples, 9,835 items | 45.8 / 46.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_live_hypotheses: native rows+dense 125,981+0 -> 125,981+0
- te_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_data: native rows+dense 112,164+16,412 -> 112,164+16,412

### stream-data (survey rank 2)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_data | 200 | 193.3 | 229.0 (209.2-304.6) | 78.7 (69.9-85.6) | 227.4 | **3.20x** (2.52-3.60x, 7/7) | 2.43x (2.17-2.99x, 7/7) | 7,377 | 44.1 / 44.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_mil_run | 200 | 21.7 | 37.3 (35.4-42.4) | 22.8 (21.6-27.3) | 41.9 | **1.69x** (1.30-1.87x, 7/7) | 1.00x (0.79-1.24x, 3/7) | 201 | 42.1 / 42.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_data | 200 | 62.9 | 76.8 (73.7-86.5) | 34.0 (29.3-39.3) | 89.0 | **2.25x** (1.88-2.79x, 7/7) | 1.85x (1.61-2.96x, 7/7) | 1,653 | 44.0 / 44.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_data | 40 | 25,681.9 | 30,624.3 (23,741.0-44,216.9) | 8,832.5 (8,242.7-11,193.6) | 29,280.4 | **2.88x** (2.17-5.01x, 7/7) | 2.78x (2.24-3.47x, 7/7) | 173,693 | 48.8 / 51.9 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_data | 200 | 297.7 | 332.4 (297.0-361.8) | 100.3 (86.0-148.9) | 333.2 | **3.42x** (2.11-3.93x, 7/7) | 2.83x (2.01-3.53x, 7/7) | 9,835 | 46.3 / 46.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_mil_run: native rows+dense 109,210+0 -> 109,210+0
- te_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_data: native rows+dense 112,164+16,412 -> 112,164+16,412

### collapse-filter (survey rank 3)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_filter | 200 | 1,091.4 | 1,097.8 (997.5-1,268.3) | 54.4 (53.1-62.5) | 1,015.6 | **18.85x** (16.74-23.44x, 7/7) | 18.77x (15.43-23.17x, 7/7) | 200 tuples, 7,268 items | 44.8 / 45.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_base_rate_rows | 200 | 136.8 | 187.3 (167.7-196.3) | 38.4 (35.1-42.0) | 184.0 | **4.86x** (4.00-5.46x, 7/7) | 3.54x (3.26-4.86x, 7/7) | 200 tuples, 66 items | 90.7 / 91.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_filter | 200 | 270.8 | 297.2 (272.6-360.3) | 29.3 (27.9-34.5) | 304.7 | **9.95x** (7.91-12.28x, 7/7) | 9.29x (6.88-10.11x, 7/7) | 200 tuples, 1,620 items | 44.8 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_filter | 40 | 126,383.4 | 131,071.7 (127,961.7-145,996.1) | 6,871.4 (5,764.8-12,634.7) | 131,959.3 | **19.06x** (11.56-23.58x, 7/7) | 18.38x (9.50-21.36x, 7/7) | 40 tuples, 173,693 items | 46.9 / 61.5 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_filter | 200 | 1,466.3 | 1,459.3 (1,306.8-1,648.1) | 81.7 (72.7-89.7) | 1,383.6 | **18.36x** (16.00-19.09x, 7/7) | 17.92x (14.10-21.06x, 7/7) | 200 tuples, 9,643 items | 46.6 / 47.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_base_rate_rows: native rows+dense 105,107+0 -> 105,107+0
- te_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

### stream-filter (the probe's shape (survey rank 8), kept for the probe sites)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_filter | 200 | 976.6 | 1,096.2 (968.2-1,567.6) | 83.8 (65.8-132.2) | 1,007.5 | **12.75x** (11.78-14.71x, 7/7) | 11.57x (7.10-14.84x, 7/7) | 7,268 | 44.0 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_filter | 200 | 259.4 | 281.8 (252.3-311.3) | 33.3 (31.1-35.8) | 283.0 | **8.51x** (7.25-10.01x, 7/7) | 7.60x (6.96-10.42x, 7/7) | 1,620 | 44.6 / 44.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_filter | 40 | 123,641.9 | 132,531.1 (129,707.8-138,890.5) | 9,078.0 (7,686.5-9,632.9) | 128,225.2 | **15.07x** (13.64-17.34x, 7/7) | 13.69x (12.59-16.47x, 7/7) | 173,693 | 48.6 / 61.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_filter | 200 | 1,448.6 | 1,679.1 (1,288.8-1,855.2) | 111.6 (91.5-143.2) | 1,532.9 | **13.67x** (11.10-19.24x, 7/7) | 12.32x (9.87-16.88x, 7/7) | 9,643 | 46.3 / 46.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- te_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

### Checks

- every site: the same answers per batch (and answer items) in all four modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call

Recompute: `make fnspec-recompute` regenerates the summary from the raw JSONL and compares it byte for byte with
evidence/fnspec-2026-11-18/bench/summary.md.

## Limits

- Shared host (load average 3.0-4.8 during the benchmark, env.txt), one machine, seven processes per mode. The
  ranges in the tables are wide on the largest sites (tf: fn-hosted 23.7-44.2 ms per query); the paired ratios were
  > 1 in 7/7 runs everywhere except mil:run vs fn-generic (3/7).
- Probe sites use the M1 synthetic KB; the real-code sites run their real function on synthetic rows of the right shape
  (bench/gen_fnspec_kb.py); the real programs fill these spaces at run time and their contents were not available.
  mil:run's KB gives about one answer per call (201 answers over 200 calls); a richer KB would move it towards the
  stream-data probe sites.
- PeTTaChainer's functions are PeTTa code; here they run under CeTTa's he language with their type declarations
  dropped. For this fragment (match, collapse, if, ==, =alpha, and, or, empty) the bag tests show CeTTa's he semantics;
  PeTTa's own runtime was not compared.
- fn-hosted uses this milestone's own hosted callable (cpk-hosted:f, CeTTa's conjunction matcher behind an embedding
  callback), not the M1 pack's hosted-generic; on the probe shape it times like the probes' fn-hosted (282 vs 258 us on
  M1's site). There is no fn-kernel column: the PathMap kernel is outside this milestone (stop-line).
- The definition guard compares atoms structurally (variables by bijection). A token rebound with bind! to another
  grounded value between the definition and fs_open would make the compiled atoms differ from the stored ones and
  send calls to the residual (safe, not tested). The builtin guard compares the equations and declarations of each
  builtin with a fresh engine's; it does not detect a change inside CeTTa's C implementation of a builtin (that is the
  build/profile identity, the same binary here).
- Normal form is checked conservatively (any symbol with a possibly matching equation in &self, any builtin or
  grounded-operation head, any variable in an answer sends the call to the residual), so some calls CeTTa would answer
  the same way take the residual; this costs speed, never answers.
- Collapse answers are compared as CeTTa returns them, row order included; that order is the matcher's
  (space_query_conjunction) and matched CeTTa's evaluator on every test. Nested matches under collapse are refused
  (their flattened row order is not established); collapse(once) and existence tests (survey ranks 4-7) are not
  compiled.
- The entry point is an embedding callback (stop-line); match-rows-style entry (0003) would remove the callback
  hand-back measured in the probes but is not used here.
- Revalidation cost (a few MeTTa queries of &self per program change) is paid once per change and is not in the
  timed loop; code that redefines functions on every call would pay it per call (not measured).
