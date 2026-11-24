# fnspec 2: existence tests and collapse(once), imported-module guard, match-rows build

Task t-2026-11-19-cettapack-fnspec-2 (andy, order projects/cettapack/orders/2026-11-19-cettapack-fnspec-2.md).
Worktree wt/fnspec-2, branch hive/t-2026-11-19-cettapack-fnspec-2, base cettapack main **bc4732b** (fnspec merged,
PR 5). CeTTa a9b3a811 + upstream/cetta 0001-0003, unchanged, as files (build tree head 383d2a20). Method and terms as
in docs/FNSPEC.md and docs/FNSPEC_SURVEY.md. Evidence: evidence/fnspec2-2026-11-19/.

**Question.** Can the fnspec compiler take the survey's next two shapes, existence tests and collapse(once), with
the same guarantees; does its definition guard also cover functions from imported modules; and (if cheap) what do the
fnspec sites gain through match-rows?

**Answer.** (1) Yes: four new shapes (once-data, once-filter, exist-data, exist-filter) compile with fnspec's guards
plus a check that once is CeTTa's first-answer builtin in the engine's profile; bag tests against CeTTa evaluating the
same function pass on every site and space (47 sites, 1,551 checks, 0 failures, forced-INVALID and every redefinition
step taking the residual), and fn-spec is faster than CeTTa on all 21 benchmark sites (7/7 paired runs on 20, 5/7 on
the 21st):

| shape (survey rank) | fn-spec vs CeTTa evaluating the function (fn-generic), paired median, probe-style sites | fn-spec vs fn-hosted | real-code site (PeTTaChainer): vs CeTTa, vs hosted |
|---|---|---|---|
| existence test + filter (4) | **9.8x-36.5x** | 9.8x-39.9x | sk-needed?: **11.9x**, 13.0x |
| collapse(once) + data (5) | **2.5x-6.8x** | 4.0x-11.8x | stored-proof-tvs (about 0.5 rows per call): **1.32x** (5/7 runs faster), 2.8x |
| existence test + data (6) | **3.0x-5.1x** | 3.7x-8.6x | given-rule?: **2.0x**, 3.8x; compound-connective? (no once): **2.1x**, 3.6x |
| collapse(once) + filter (7) | **9.4x-40.7x** | 11.0x-41.8x | the collapse(once) of versions.metta:1247: **11.1x**, 13.7x |

As in fnspec, the gain follows what the compiled code removes: a filter costs CeTTa an if, an == and an (empty) per
row, and the compiled condition stops at the first surviving row (10-41x); with a data template CeTTa's cost is the
evaluation of the row and its wrapper (2-7x); where a call has about one row (stored-proof-tvs) there is little to
remove. Guards are inside every fn-spec time; peak RSS of fn-spec is within 0.5 MiB of fn-hosted's or below it on every site (no index).
(2) The guard catches redefinitions of a function that comes from an imported module (one and two import levels,
changes in the module's own space, a second module imported): tested, no change to the guard needed. (3) The
match-rows build was not built: match-rows has no hook for compiled code, so the condition, the per-answer
normal-form check and the first-row cut would be lost (section 5).

## 1. The two shapes, as the survey defines them

tools/survey_shapes.py (docs/FNSPEC_SURVEY.md, Table 2) classifies a match by what the body does with its answers:

- **collapse(once)**: (collapse (once (match S P T))): the first answer, as a tuple (or () when there is none);
  survey rank 5 (data template, 28 occurrences) and rank 7 (filter template, 19).
- **existence test**: a collapse or collapse(once) compared with (): (not (== W ())) (some row) or (== W ())
  (no row); rank 4 (filter, 28) and rank 6 (data, 23).

The compiler (tools/fnspecc.py) now accepts, besides fnspec's four shapes:

    once-data      (collapse (once (match S P T)))
    once-filter    (collapse (once (match S P (if C T (empty)))))         either branch may be the (empty) one
    exist-data     (not (== (collapse [(once] (match S P T)[)]) ()))     also (== W ()) and (== () W)
    exist-filter   the same with the filter template

with S, P, T and C as in fnspec (S an argument or a named space token, P one pattern or a flat conjunction, T data,
C a tree of and/or/not over ==, =alpha, <, >, <=, >= on bound values, arguments and ground constants). A nested
match under collapse is still refused. Anything else is refused with a reason and stays CeTTa's. The shapes are
compiled for any function of that form (the num_* and imp_* test sites are compiled by the same code), not only for
the benchmark sites.

**once is a profile property in CeTTa.** In CeTTa's default he profile, once is not a builtin: (once X) is data and
(collapse (once (match ...))) returns ((once v1) (once v2) ...), every row wrapped
(evidence/fnspec2-2026-11-19/semantics/: the same file run with --lang he under the profiles he, he-compat,
extended, he-prime, and in PeTTa). In the extended and he-prime profiles, and in PeTTa, once is first-answer.
PeTTaChainer is PeTTa code and means first-answer, so the sites that use once run in profile extended (a
";; profile:" line in the site file; the host opens every engine of that site, reference and compiled, in that
profile). The runtime does not trust the profile line: a plan whose body uses once runs a known-answer probe in the
engine at open time, (collapse (once (superpose (a b)))) must give (a), and otherwise the plan is INVALID for the
engine's life (every call takes the residual). The bag tests open each once site in the default he profile to show
this ("profile-he" step: all calls residual, CeTTa's answers of that profile).

**Runtime** (src/cpk_fnspec.c). The same call path as fnspec (embedding callback cpk-fnspec:f; CeTTa's conjunction
matcher on the live space, arguments as seed bindings; generated row function; guards; residual = CeTTa evaluating
(f args...) in the same engine). New: for once and existence shapes the row loop stops at the **first surviving
row** in the matcher's order (rows the filter drops are skipped; a row that meets a value outside the compiled kinds
before it aborts the call to the residual). once shapes return (answer) or (); existence shapes return the engine's
own True/False (the values (== () ()) and (== () (x)) give in a fresh engine of the same profile). The answer of the
first surviving row passes the normal-form check, as every answer does in fnspec; for an existence test this matters
too (a template that CeTTa would rewrite to nothing would make CeTTa's collapse empty).

An existence test without once compiles to the same cut: CeTTa's collapse evaluates every row, but once one row's
template is a value in normal form the tuple is non-empty whatever the other rows give (in MeTTa an error is an atom,
so a later row can add an item, never remove one). The bag tests check this on spaces with rows of mixed kinds
(num_exist_filter, rs_compound_connective).

**Guards**, as in fnspec plus two additions, in the order a call meets them: forced INVALID; once available in this
engine (new, fixed at open); the definition per &self revision (one equation of f, alpha-equal to the compiled one; no
declaration of f); builtin meanings (now including once/1, ==/2, not/1 where the body uses them; their equations and
declarations in &self equal a fresh engine's of the same profile); arguments; value kinds per row; normal form per
answer. Residual on any failure.

## 2. Bag tests (make fnspec-difftest, part of make test)

Per site, each mode in its own engine against CeTTa evaluating the same function (fn-generic): fn-hosted, fn-spec,
fn-fallback (forced INVALID), on small spaces, then the site's benchmark KB with its benchmark queries; on the first
small space the mutation sequence of docs/FNSPEC.md (second equation added/removed, redefined/restored, builtin
shadowed/unshadowed, declaration added/removed, row-not-normal, each with its expected route), recursion depth
1/10/100/1000, and for once sites the profile-he step. New small spaces (tests/fnspec_cases.py): given_rules,
logic_config, sk_needed, kb_proofs (several rows per key with the first row not the only one, rows the filter drops
before a surviving row, duplicates, expressions, strings and numbers in keys, absent keys; a "mixed" space each with
variables in rows, non-ground arguments and mixed numeric kinds). The M1 probe spaces d1-d6 and the numeric edge
spaces (INT64_MAX/MIN, -0.0/0.0, mixed int/float, bigint, symbols/strings/True compared with <) serve the probe-style
and num_* sites.

Result (evidence/fnspec2-2026-11-19/difftest/, one file per site; make-fnspec-difftest.log): **47 sites, 1,551 checks,
0 failures**: 843 mode-against-reference checks, 497 mutation steps, 188 recursion checks, 23 profile-he checks.
The 47 are fnspec's 21 sites (answers unchanged), the 21 benchmark sites of this task, num_once_filter,
num_exist_filter, and the three imported-module sites of item 2.

On the new shapes: fn-fallback (forced INVALID) took the residual on every call of every space; in every mutation
sequence the guard took the residual exactly on the steps that change the function (second equation, redefinition),
a builtin it uses or its declaration, and specialised again when the change was undone; "row-not-normal" sent only the
affected calls to the residual (e.g. te sites 3-4 of 17; rs_sk_needed 9 of 9, conservatively, because the mutation's
equation head sk also occurs in the query arguments); the "mixed" spaces sent the calls that met a variable in a row,
a non-ground argument or a mixed numeric comparison to the residual (e.g. num sites 8/8, rs_sk_needed 4/7,
rs_stored_proof_tvs 3/4) with CeTTa's answers. collapse(once) answers are compared as CeTTa returns them, so the first
row the compiled code takes is CeTTa's first row on every query of every space, including the benchmark KBs (e.g.
tf_once_data: 40 queries over 128,576 atoms). The probes' own difftest (make probe-difftest) and fnspec's 21 sites
still pass in make test: earlier answers unchanged.

## 3. Item 2: functions from imported modules

**Answer: the definition guard catches it; no guard change was needed.** Three test-only sites load the compiled
function from a module instead of &self (";; load: import" / "import2" in the site file; the host writes the
definitions to a module file, runs !(import! &self "<module>"), and binds &cpkmod to the module's own space by a
second import!, which CeTTa resolves to the same instance):

- imp_collapse_data: fnspec 1's collapse-data function, module imported by &self;
- imp_exist_filter: this task's exist-filter function, module imported by &self;
- imp2_once_data: this task's once-data function in module B, imported by module A, A imported by &self (CeTTa's
  imports are transitive here: B's function is callable from &self).

Their mutation sequence adds, on top of the &self steps (where "redefined" becomes "a second definition added in
&self", since &self cannot remove a module's equation), the same changes made **in the module's own space**
(17 queries each; spec/residual = calls specialised / taking the residual):

| step | expected | imp_collapse_data | imp_exist_filter | imp2_once_data |
|---|---|---|---|---|
| module-equation-added (a second equation of f in the module) | residual | 0/17 | 0/17 | 0/17 |
| module-equation-removed | specialised | 17/0 | 17/0 | 17/0 |
| module-redefined (f's equation replaced in the module) | residual | 0/17 | 0/17 | 0/17 |
| module-restored | specialised | 17/0 | 17/0 | 17/0 |
| module-declaration-added | residual | 0/17 | 0/17 | 0/17 |
| module-declaration-removed | specialised | 17/0 | 17/0 | 17/0 |
| module-builtin-shadowed | residual | 0/17 | 0/17 | 0/17 |
| module-builtin-unshadowed | specialised | 17/0 | 17/0 | 17/0 |
| self-redefinition-added (a second definition in &self) | residual | 0/17 | 0/17 | 0/17 |
| second-module-imported (another module defining f imported into &self) | residual | 0/17 | 0/17 | 0/17 |

Bags equal to the reference on every step; where the step changes the function the reference's answers changed on
17 of 17 queries (ref_changed in the difftest files), so the steps are real changes. Why it works (CeTTa a9b3a811,
src/space.h): a module's space records the spaces importing it, and a mutation there advances the importers'
equation and declaration revisions, which are part of the &self program token the guard watches; and the guard's
definition query (match &self (= (f ...) $b) ...) reads &self's composed view, module equations included. The guard
therefore depends on that CeTTa property; the tests above (in make test) fail if it stops holding.

## 4. Benchmark

Method as fnspec (docs/FNSPEC.md): pop-os (i7-1165G7, 8 threads, 15 GiB, shared), BUILD=mork; RUNS=7 processes per
mode, REPEAT=5 batches of all queries per process, the process's median batch, modes interleaved run by run
(bench/run_fnspec.sh); every query atom built before the timed loop; one untimed warm-up call; paired ratio = run i
of the reference over run i of fn-spec (> 1: fn-spec faster). Probe-style sites: the M1 KB (bench/gen_kb.py --seed
20261106, 128,576 atoms) with the probes' seeds; real-code sites: bench/gen_fnspec_kb.py --seed 20261118 (new kinds
given_rules, logic_config, sk_needed, kb_proofs; inputs.txt has the hashes). Modes fn-generic, fn-hosted, fn-spec,
fn-fallback as in fnspec. Structural checks per site (bench/check_fnspec_site.py, checks.txt) as in fnspec; a new
cross-site check in the summary: on the same queries, an existence test's True answers must equal the non-empty
collapse(once) tuples of the same match (te/gv/tf/vi data and filter pairs, rs_sk_needed vs rs_sk_needed_depth).

Real-code sites (PeTTaChainer 33cca343; PeTTa type declarations dropped, run in CeTTa's he language):

| shape | site | function | location |
|---|---|---|---|
| exist-data (with once) | rs_given_rule | given-rule? | pettachainer/metta/runtime/store/rule_identity.metta:21 |
| exist-data (no once, default profile) | rs_compound_connective | compound-connective? | pettachainer/metta/core/logic_config.metta:49 |
| exist-filter (with once) | rs_sk_needed | sk-needed? | pettachainer/metta/runtime/evaluator/versions.metta:1242 |
| once-data | rs_stored_proof_tvs | stored-proof-tvs | pettachainer/metta/runtime/proofs.metta:150 |
| once-filter | rs_sk_needed_depth | (the collapse(once) argument of the function at that line) | pettachainer/metta/runtime/evaluator/versions.metta:1247 |

The survey has no whole-function once-filter body (rank 7: 19 occurrences, 0 whole bodies); rs_sk_needed_depth is
the (collapse (once (match &sk_needed ... (if (=alpha $stored $key) $depth (empty))))) that the function at
versions.metta:1247 passes to chainer-first-or, compiled as a function of its own.

Tables (bench/summarize_fnspec.py on evidence/fnspec2-2026-11-19/bench/runs-*.jsonl; the same file as
evidence/fnspec2-2026-11-19/bench/summary.md, which make fnspec-recompute regenerates and compares, for fnspec's
evidence and this one):

### exist-filter (existence test + filter, survey rank 4)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_filter | 200 | 904.3 | 965.9 (872.1-1,074.6) | 38.4 (35.1-42.4) | 883.8 | **24.91x** (23.17-26.36x, 7/7) | 23.40x (19.19-25.79x, 7/7) | 200 answers, 190 True | 45.4 / 45.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_needed | 200 | 547.4 | 642.6 (518.8-689.3) | 46.3 (40.0-53.6) | 600.6 | **12.96x** (10.76-16.26x, 7/7) | 11.93x (10.09-13.08x, 7/7) | 200 answers, 100 True | 50.3 / 50.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_filter | 200 | 242.0 | 245.5 (219.1-294.3) | 26.0 (24.4-27.3) | 245.2 | **9.79x** (8.27-10.83x, 7/7) | 9.83x (8.57-12.18x, 7/7) | 200 answers, 190 True | 45.5 / 45.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_filter | 40 | 98,554.3 | 114,242.9 (107,668.8-135,048.8) | 3,214.8 (2,254.3-4,212.2) | 103,443.0 | **39.89x** (25.56-51.63x, 7/7) | 36.49x (22.50-47.34x, 7/7) | 40 answers, 38 True | 45.7 / 64.5 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_filter | 200 | 1,176.8 | 1,275.7 (1,149.5-1,470.1) | 54.2 (49.1-60.5) | 1,198.6 | **23.56x** (22.37-24.32x, 7/7) | 21.95x (18.81-25.05x, 7/7) | 200 answers, 189 True | 47.1 / 47.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_needed: native rows+dense 105,081+0 -> 105,081+0
- te_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

### once-data (collapse(once) + data, survey rank 5)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_data | 200 | 181.2 | 296.9 (253.7-434.3) | 43.6 (35.8-56.3) | 207.2 | **6.86x** (5.27-12.14x, 7/7) | 4.14x (3.47-4.71x, 7/7) | 200 tuples, 190 non-empty | 44.4 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_stored_proof_tvs | 200 | 33.6 | 68.8 (61.8-77.0) | 25.5 (22.1-45.3) | 58.7 | **2.76x** (1.52-3.16x, 7/7) | 1.32x (0.73-1.41x, 5/7) | 200 tuples, 102 non-empty | 50.7 / 50.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_data | 200 | 65.6 | 102.4 (90.3-119.1) | 25.4 (24.0-29.0) | 89.3 | **4.02x** (3.38-4.53x, 7/7) | 2.51x (2.20-4.09x, 7/7) | 200 tuples, 190 non-empty | 44.0 / 44.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_data | 40 | 21,755.1 | 33,941.8 (27,694.5-40,130.0) | 3,181.7 (2,550.5-5,555.2) | 21,863.6 | **11.78x** (5.98-14.19x, 7/7) | 6.84x (3.86-9.89x, 7/7) | 40 tuples, 38 non-empty | 45.0 / 61.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_data | 200 | 279.2 | 395.5 (354.1-529.0) | 58.0 (49.8-120.3) | 278.9 | **6.69x** (3.43-10.63x, 7/7) | 4.58x (2.27-7.60x, 7/7) | 200 tuples, 189 non-empty | 46.0 / 46.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_stored_proof_tvs: native rows+dense 126,089+0 -> 126,089+0
- te_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_data: native rows+dense 112,164+16,412 -> 112,164+16,412

### exist-data (existence test + data, survey rank 6)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_data | 200 | 177.6 | 221.6 (210.5-250.4) | 40.2 (36.0-43.5) | 200.2 | **5.98x** (5.07-6.26x, 7/7) | 4.40x (4.09-5.01x, 7/7) | 200 answers, 190 True | 44.8 / 45.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_compound_connective | 200 | 46.3 | 79.1 (66.0-87.2) | 21.8 (18.9-22.4) | 69.2 | **3.63x** (3.05-3.97x, 7/7) | 2.13x (2.03-2.48x, 7/7) | 200 answers, 107 True | 21.7 / 22.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_given_rule | 200 | 45.2 | 79.9 (72.3-88.8) | 23.1 (19.8-27.4) | 69.6 | **3.81x** (2.92-3.99x, 7/7) | 2.01x (1.65-2.43x, 7/7) | 200 answers, 102 True | 76.6 / 76.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_data | 200 | 82.1 | 108.7 (96.3-112.3) | 26.3 (20.6-42.4) | 107.2 | **3.66x** (2.57-5.31x, 7/7) | 2.98x (1.94-5.69x, 7/7) | 200 answers, 190 True | 44.7 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_data | 40 | 17,809.3 | 26,828.3 (23,094.5-32,929.0) | 3,398.2 (2,702.9-5,534.8) | 17,609.7 | **8.56x** (5.95-9.47x, 7/7) | 5.08x (3.24-6.88x, 7/7) | 40 answers, 38 True | 45.2 / 57.8 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_data | 200 | 256.2 | 336.3 (298.5-420.7) | 57.7 (52.3-82.4) | 276.3 | **5.55x** (4.08-8.04x, 7/7) | 4.24x (2.99-8.88x, 7/7) | 200 answers, 189 True | 46.5 / 46.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_compound_connective: native rows+dense 6,423+18,841 -> 6,423+18,841
- rs_given_rule: native rows+dense 104,917+0 -> 104,917+0
- te_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_data: native rows+dense 112,164+16,412 -> 112,164+16,412

### once-filter (collapse(once) + filter, survey rank 7)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_filter | 200 | 1,130.4 | 1,229.3 (1,045.4-1,954.5) | 43.0 (39.7-87.3) | 1,190.3 | **26.33x** (12.21-48.70x, 7/7) | 24.73x (13.01-33.10x, 7/7) | 200 tuples, 190 non-empty | 44.0 / 45.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_needed_depth | 200 | 593.9 | 635.1 (513.6-1,131.1) | 51.7 (44.7-54.3) | 591.9 | **13.65x** (10.74-21.86x, 7/7) | 11.11x (9.18-19.33x, 7/7) | 200 tuples, 100 non-empty | 49.6 / 49.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_filter | 200 | 267.2 | 303.9 (266.1-334.1) | 27.4 (23.8-46.8) | 293.6 | **11.00x** (6.88-11.87x, 7/7) | 9.36x (5.94-11.73x, 7/7) | 200 tuples, 190 non-empty | 44.4 / 45.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_filter | 40 | 135,753.7 | 142,987.9 (133,542.2-149,994.2) | 3,420.7 (2,225.1-3,567.0) | 134,214.5 | **41.80x** (37.44-67.41x, 7/7) | 40.72x (35.48-70.74x, 7/7) | 40 tuples, 38 non-empty | 45.0 / 67.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_filter | 200 | 1,371.6 | 1,599.0 (1,385.8-1,802.0) | 56.2 (45.0-60.6) | 1,520.3 | **29.71x** (24.53-40.02x, 7/7) | 25.14x (22.51-31.22x, 7/7) | 200 tuples, 189 non-empty | 46.5 / 47.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_needed_depth: native rows+dense 105,081+0 -> 105,081+0
- te_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

### Checks

- existence test agrees with collapse(once): te_exist_data 190 True = te_once_data 190 non-empty
- existence test agrees with collapse(once): gv_exist_data 190 True = gv_once_data 190 non-empty
- existence test agrees with collapse(once): tf_exist_data 38 True = tf_once_data 38 non-empty
- existence test agrees with collapse(once): vi_exist_data 189 True = vi_once_data 189 non-empty
- existence test agrees with collapse(once): te_exist_filter 190 True = te_once_filter 190 non-empty
- existence test agrees with collapse(once): gv_exist_filter 190 True = gv_once_filter 190 non-empty
- existence test agrees with collapse(once): tf_exist_filter 38 True = tf_once_filter 38 non-empty
- existence test agrees with collapse(once): vi_exist_filter 189 True = vi_once_filter 189 non-empty
- existence test agrees with collapse(once): rs_sk_needed 100 True = rs_sk_needed_depth 100 non-empty
- every site: the same answers per batch (and answer items) in all four modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call

## 5. Item 3: match-rows build (optional): not built

Not cheap, and not possible with the same guarantees, so skipped. 0003's (match-rows S P R) runs CeTTa's conjunction
matcher and returns R instantiated per row **as data**; it has no hook for compiled code. A build that "runs the
fnspec sites through 0003" could therefore only replace the match by match-rows and leave the rest of the body to
CeTTa, with a guard callback in front:

- the compiled condition (filter shapes) would be evaluated by CeTTa again, so the filter sites would lose what fnspec
  gains on them (the condition in C);
- the per-answer normal-form check could not run (match-rows hands rows straight to the evaluator), so a row CeTTa
  would rewrite would be returned unrewritten: the row-not-normal mutation step would fail by design, against "bag
  tests still 0 failures" (0003's own note: match-rows agrees with match only on rows in normal form);
- the first-surviving-row cut of the once and existence shapes would be lost (match-rows produces every row).

Doing it properly needs a new CeTTa entry, e.g. a match-rows variant that calls a registered native row function per
row inside the evaluator (a 0004 patch, its tests and a host for it): more than half a day, and a change to the patch
series rather than a build option. For the new shapes the gain would also be small: an existence test or
collapse(once) hands back one atom per call, so the per-answer hand-back the probes measured (match-rows 1.4-2.4x
over the callback for bare match, docs/NEXT_STEP_PROBES.md) is mostly absent; for stream shapes it would matter more.

## Limits

- Shared host, one machine, seven processes per mode (env.txt: load average at start and per site).
- Probe-style sites use the M1 synthetic KB; the real-code sites run their real function on synthetic rows of the
  right shape (bench/gen_fnspec_kb.py); the real programs fill these spaces at run time. PeTTaChainer's functions are
  PeTTa code run under CeTTa's he language (profile extended where once is used); PeTTa's own runtime was not compared.
- The compiled code computes every row of the match (space_query_conjunction has no bounded form) and stops after the
  first surviving one only in its own row loop (condition, template, normal form); a bounded matcher entry would make
  the existence and once shapes cheaper still. CeTTa's once (profile extended) also appears to compute every row on
  these sites (fn-generic times of the once/exist sites are close to fnspec's collapse sites on the same pattern).
- First-row order: the compiled once shapes take the first surviving row of space_query_conjunction; that equals
  CeTTa's first row on every bag-test and benchmark query, but it is CeTTa's evaluation order, not a documented
  guarantee (CeTTa's own docs say collapse/reify promise no traversal order).
- The imported-module guard relies on CeTTa advancing importers' revisions on a module mutation; tested for one and two
  import levels and for a second import; not tested: a space attached to &self by other means than import!, and
  imports in the PeTTa language.
- Normal form stays conservative (any symbol with a possibly matching equation in &self sends the call to the
  residual), so some calls CeTTa would answer the same way take the residual; this costs speed, never answers.
