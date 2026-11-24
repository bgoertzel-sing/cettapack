# fnspec 3: live == probe, the if/case template shapes, match-rows row hook (0004)

Task t-2026-11-24-cettapack-fnspec-3 (andy, 2026-11-24; order projects/cettapack/orders/2026-11-24-cettapack-fnspec-3.md).
Base: cettapack main 1e118a3 (PR 6, fnspec 2, merged). CeTTa a9b3a811 + upstream/cetta 0001-0003 as files, plus the
new 0004 (this task). Method and terms: docs/FNSPEC.md, docs/FNSPEC_2.md, docs/FNSPEC_SURVEY.md.

## Summary

**Item 1.** Existence tests now probe == (and not) in the live engine at open, as once was probed: the test the body
makes, with an empty and a non-empty W, in both operand orders; any answer other than he's True/False (or a probe
that fails) leaves the plan INVALID and every call takes the residual. In a PeTTa-language engine (which answers
false/true) all 28 forced-mismatch tests take the residual and give the engine's own answers; in he engines the
probe passes and every exist site still specialises.

**Item 2.** After ranks 1-8 (fnspec, fnspec 2), the survey's next counts are rank 9 collapse(once) + if/case/let (4),
rank 10 fold/count + data (3), rank 11 existence test + if/case/let (3), rank 12 stream + if/case/let (3). Ranks 9, 11
and 12 are one template family under answer forms already compiled, so one extension, a compiled template tree
(nested if, case on a bound value, (empty), data, boolean leaves), covers all three (10 occurrences, 7 accepted).
Rank 10 is not compiled: all three occurrences are match-count with the pattern as an argument, outside fnspec's
fixed-pattern method. New shapes once-cond, exist-cond, stream-cond (and collapse-cond, same compiler): 16
probe-style and 4 PeTTaChainer sites. Bag tests 0 failures (67 sites, 3,673 checks, both entry paths, forced INVALID
and mid-run redefinition taking the residual). Against CeTTa evaluating the same function: 4.5x-102x on the
probe-style sites; on PeTTaChainer code 3.8x (the case of ledger.metta:146) and 2.7x (hypothesis_mining.metta:83),
but 0.95x and 0.89x on core.metta:258 and skeleton.metta:643, where CeTTa's matcher takes about 1-2 ms a query on
those row shapes and dominates both (section 2.5).

**Item 3.** 0004 gives match-rows a row hook: the matcher's rows go to the compiled code inside match-rows, with the
same guards, condition, per-answer normal-form check and first-row cut, and the residual evaluated in place of a
refused call. Its first revision was slower than the callback (0.65x-0.97x on six sites) because the matcher ran on
the call's instantiated pattern; in the committed revision the hook supplies its own compiled query. Through it,
every one of the 60 benchmark sites gives the callback's answers; paired against the callback the hook is
1.14x faster at the median (0.83x-1.75x; faster on 58 of 60), most where the call overhead is a large part of the
time (the 12 PeTTaChainer/CeTTa-library sites: 1.03x-1.75x, median 1.31x), at par where the matcher dominates
(the tf sites, 0.83x-1.15x, and the two 1-2 ms real sites).
Nothing went to Zar's CeTTa.


## 1. Item 1: the live == probe

**Before.** An existence-test plan answered he's True/False, read from a fresh he engine at open
(src/cpk_fnspec.c fs_open), while the live engine was never asked. once already had a known-answer probe in the
live engine (fnspec 2). Finding of the fnspec 2 review: an engine whose == or not answers other atoms (a
PeTTa-language engine answers true/false) would get he's atoms from the compiled code and different ones from CeTTa.

**Now.** At open, for every plan with an existence test, the runtime evaluates in the live engine the test the body
makes, with an empty and a non-empty W and in both operand orders: (not (== () ())), (not (== (cpk-exist-x) ())),
(not (== () (cpk-exist-x))), (not (== () ())) for (not (== W ())); (== () ()), (== (cpk-exist-x) ()), ... for
(== W ()). Each must give exactly one answer, the same atom the compiled code returns in that case (he's True or
False from the fresh engine). For plans whose template answers a boolean (fnspec 3's tree shapes), the live engine
is also asked (== cpk-a cpk-a), (== cpk-a cpk-b), (not (== cpk-a cpk-b)), (and True False), (or False True).
Any difference, a count other than one, or a failed probe sets eq_mismatch: the plan is INVALID for the engine's
life (revalidate refuses first thing, reason "the engine's existence-test answers (== / not probe) differ from he's
True/False"), and every call takes the residual. Like once's probe, a probe that fails for any other reason (a
shadowed == at open time) leaves the plan INVALID, never wrongly valid; a change to == after open is the builtin
guard's business, as before.

**Tests** (in make fnspec-difftest, for every exist site; src/cpk_fnspec_host.c language_test, eq_probe_check):

- language-petta (the forced mismatch): the site's definitions loaded into a PeTTa-language engine; fn-generic in
  one PeTTa engine against the compiled callable in another. Passes when eq_mismatch is set, every call took the
  residual (spec=0, residual=queries) and the bags equal the PeTTa engine's own. Run for both the callback
  (fn-spec) and the match-rows hook (fn-rows). Named-space exist sites (rs_sk_needed, rs_sk_row_present,
  rs_given_rule, rs_compound_connective) are skipped with a note: a PeTTa engine does not resolve their bind!
  token through quote as the he host does; the 12 space-argument exist sites run it.
- eq-probe in the site's engine: the probe passes in the site's own he engine (eq_mismatch=no); the matching
  engine still specialises (the fn-spec rows of every exist site: spec = queries, residual 0).

Results: evidence/fnspec3-2026-11-24/difftest (section 3). 28 language-petta tests (the 14 space-argument existence-test sites of fnspec 2 and 3, gv/te/tf/vi x
exist-data/filter/cond, imp_exist_filter and num_exist_filter, each in the callback and the hook path) pass: in the
PeTTa engine every call took the residual (spec=0, residual=queries, eq_mismatch=yes, guard reason as above), and the
answers are the engine's own (false/true atoms, e.g. te_exist_cond: 17 queries, first answer false), bag-equal to
CeTTa evaluating the function in a PeTTa engine. 23 eq-probe checks pass in the sites' own he engines (every exist
site and every boolean-leaf tree site), and every exist site's fn-spec and fn-rows rows show spec = queries,
residual 0 on its non-mixed cases (the matching engine still specialises).

## 2. Item 2: the next shapes

### 2.1 Which, from the survey's counts

Compiled before this task (docs/FNSPEC_SURVEY.md Table 2, compilable families): ranks 1-3 (fnspec), 4-7 (fnspec 2)
and rank 8, stream + filter (the probes' shape, kept since the probes). The next ranks:

| rank | answers | template | occurrences | whole-function | decision |
|---|---|---|---|---|---|
| 9 | collapse(once) | if/case/let | 4 | 0 | compiled: once-cond |
| 10 | fold/count | data | 3 | 3 | not compiled (below) |
| 11 | existence test | if/case/let | 3 | 1 | compiled: exist-cond |
| 12 | stream | if/case/let | 3 | 0 | compiled: stream-cond |

Ranks 9, 11 and 12 are one template family (if/case/let, 10 occurrences) under three answer forms that are already
compiled, so one compiler extension, a template tree, covers all three. Rank 10 is not compiled: its 3 occurrences
are the same library function, match-count (CeTTa lib/petta/lib_spaces.metta:28, lib/petta/typecheck/
lib_spaces.metta:30, PeTTa lib/lib_spaces.metta:30), (foldall + (match $space $pattern 1) 0), whose pattern is an
argument. fnspec compiles a fixed pattern; specialising it would need a pattern compiled per call (a different
method, close to M1's kernel), so it is outside the order's "same method". collapse + tree (collapse-cond) comes
free with the compiler and is tested on the four probe sites, but it is not a survey family (0 occurrences).

### 2.2 The template tree (tools/fnspecc.py compile_tree)

    T ::= (empty) | D | B | (if C T T) | (case X ((P1 T1) ... (Pn Tn)))
    C ::= (op X Y) | (and C C) | (or C C) | (not C) | True | False | (if C C C)
    B ::= a C used as the answer (he's True/False)

D is data (nothing evaluated), op one of == =alpha < > <= >= on variables bound by the row or the arguments and
ground constants (fs_compare, unchanged since fnspec: values outside the compiled kinds abort to the residual).
case's scrutinee is a variable; it must be a value (normal form; no variable), its patterns are data, matched in
order with CeTTa's simple_match after the row's bindings are applied to the pattern (as CeTTa instantiates the
template before evaluating it); the first pattern that matches is taken; the variables a pattern binds may appear
only in that branch's data leaves; no branch matching gives no answer (HE's case with no matching branch), unlike
(empty), which the stream form counts for CeTTa's Empty-per-row result (fnspec probes, d5/d6). Every node computes
only what CeTTa computes on its path (if: the condition, then one branch; case: the patterns in order until one
matches). A template that is a single (if C T (empty)) with data T is still the filter shape, compiled as before:
the generated C of every earlier site is unchanged apart from the template line (now the FS_TEMPLATE() macro, the
same bindings_apply) and three new plan fields (checked by regenerating all 47 fnspec 1/2 sites and diffing
against fnspec 2's build).

Refused, with a reason, and so left to CeTTa: a condition leaf that is not a Bool (if is (-> Bool Atom Atom $t) in
he, so PeTTa's true in a condition is a type error there), calls to other functions anywhere, let, nested match,
unify, case inside a case branch, a case scrutinee that is not a variable.

### 2.3 The ten survey occurrences

| family | location (PeTTaChainer 33cca343 unless noted) | compiler | site |
|---|---|---|---|
| once + tree | runtime/hypothesis_mining.metta:83 (case scrutinee; boolean leaf) | accepted (once-cond) | rs_hm_recurs |
| once + tree | runtime/evaluator/core.metta:258 (case scrutinee; 4-level if) | accepted (once-cond) | rs_sk_summary_memo |
| once + tree | runtime/evaluator/versions.metta:1024 (case scrutinee; 2-level if) | accepted (once-cond) | - |
| once + tree | runtime/evaluator/versions.metta:333 | refused: the condition calls evidence-excludes? | - |
| exist + tree | runtime/backward/skeleton.metta:643 (whole function) | accepted (exist-cond) | rs_sk_row_present |
| exist + tree | runtime/statistics/ledger.metta:129 | accepted (exist-cond) | - |
| exist + tree | runtime/backward/skeleton.metta:993 | refused: a condition (if (== $lifetime backward) true ...) whose leaf true is not a Bool in he | - |
| stream + tree | runtime/statistics/ledger.metta:146 (the inner match; case) | accepted (stream-cond) | rs_sk_left_out |
| stream + tree | runtime/evaluator/values.metta:828 | refused: nested match, let, copy_term | - |
| stream + tree | CeTTa examples/atp/prime/direct_search.metta:52 | refused: unify template | - |

7 of 10 accepted (the three unlisted ones checked by compiling their subexpression; not committed as sites). The
real-code sites take the function (or the collapse(once)/match subexpression the function evaluates, as fnspec 2
did for versions.metta:1247) in CeTTa's he language, profile extended where once is used (andy, 2026-11-24), on a
generated space with the pattern's row shape (bench/gen_fnspec_kb.py kinds hm_seen, sk_summary, sk_row,
sk_member).

### 2.4 Sites

Probe-style (M1 KB, the probes' four patterns gv, te, tf, vi), for each of once-cond, exist-cond, stream-cond and
collapse-cond (16 sites, fnspec/sites/{gv,te,tf,vi}_{once,exist,stream,collapse}_cond.metta):

    once-cond     (collapse (once (match $space P (if (== CV C0) (empty) (if (and (== CV C1) (not (=alpha X CV))) (hit X) (X Y))))))
    exist-cond    (not (== (collapse (once (match $space P (if (== CV C0) (empty) (if (== CV C1) (empty) true))))) ()))
    stream-cond   (match $space P (case CV ((C0 (empty)) (C1 (hit X)) ($other (X $other)))))
    collapse-cond (collapse (match $space P (if (== CV C0) (empty) (== CV C1))))

(CV the tissue, or the gene for tf; C0/C1 values present in the bag-test spaces.) Real code: rs_hm_recurs,
rs_sk_summary_memo (once-cond), rs_sk_row_present (exist-cond), rs_sk_left_out (stream-cond).

### 2.5 Benchmark (RUNS=7 REPEAT=5, interleaved, prebuilt atoms, pop-os; evidence/fnspec3-2026-11-24/bench)

bench/run_fnspec.sh on the 20 new sites, modes fn-generic, fn-hosted, fn-spec, fn-fallback and fn-rows, at 96b3fa7 (section 6). Per-query medians in us (min-max over the 7 processes); paired ratios per run (k/n: runs where fn-spec was faster). make fnspec-recompute regenerates summary.md from the raw runs (byte for byte, with fnspec 1 and 2's). Structural checks (bench/check_fnspec_site.py): every site, every mode the same answers per batch, no Error or unevaluated call among the answers, fn-spec and fn-rows specialised every timed call, fn-fallback took the residual on every one (evidence/fnspec3-2026-11-24/bench/checks.txt).

Raw runs: runs-<site>.jsonl; environment: env.txt; inputs: inputs.txt; commands: commands.txt.

#### once-cond (collapse(once) + if/case template tree, survey rank 9 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_cond | 200 | 2,702.1 | 2,687.9 (2,461.1-2,939.0) | 39.3 (36.4-85.8) | 2,707.2 | **66.39x** (34.26-75.01x, 7/7) | 64.36x (31.50-78.66x, 7/7) | 200 tuples, 190 non-empty | 45.4 / 55.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_hm_recurs | 200 | 107.7 | 153.3 (147.5-169.2) | 40.7 (37.2-43.0) | 145.3 | **3.97x** (3.53-4.20x, 7/7) | 2.68x (2.53-3.21x, 7/7) | 200 tuples, 150 non-empty | 62.9 / 63.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_summary_memo | 200 | 916.2 | 1,114.6 (1,018.1-1,592.2) | 936.2 (886.5-1,043.1) | 884.4 | **1.18x** (1.09-1.78x, 7/7) | 0.95x (0.81-1.14x, 2/7) | 200 tuples, 100 non-empty | 39.7 / 40.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_cond | 200 | 606.5 | 865.8 (614.9-1,172.8) | 27.0 (25.8-28.1) | 661.2 | **31.83x** (23.78-41.81x, 7/7) | 22.47x (19.55-28.95x, 7/7) | 200 tuples, 190 non-empty | 44.3 / 45.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_cond | 40 | 338,493.7 | 348,548.5 (307,943.2-439,913.8) | 3,303.1 (2,636.8-6,984.2) | 344,521.9 | **101.42x** (57.51-162.44x, 7/7) | 102.48x (61.02-143.50x, 7/7) | 40 tuples, 38 non-empty | 46.3 / 286.2 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_cond | 200 | 3,264.0 | 3,449.9 (3,393.9-3,695.6) | 57.5 (50.1-105.6) | 3,552.9 | **63.63x** (32.65-68.83x, 7/7) | 57.59x (32.56-65.12x, 7/7) | 200 tuples, 189 non-empty | 47.0 / 65.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_hm_recurs: native rows+dense 104,927+0 -> 104,927+0
- rs_sk_summary_memo: native rows+dense 104,907+0 -> 104,907+0
- te_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

#### exist-cond (existence test + if/case template tree, survey rank 11 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_cond | 200 | 1,532.3 | 1,528.7 (1,390.2-1,604.0) | 43.0 (37.3-70.8) | 1,491.7 | **36.01x** (19.65-41.03x, 7/7) | 36.10x (20.52-62.69x, 7/7) | 200 answers, 190 True | 45.5 / 46.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_row_present | 200 | 1,880.4 | 2,222.9 (2,083.9-2,329.9) | 2,084.7 (2,003.3-2,445.7) | 1,947.6 | **1.07x** (0.91-1.12x, 6/7) | 0.89x (0.78-0.99x, 0/7) | 200 answers, 100 True | 72.0 / 72.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_cond | 200 | 408.1 | 414.6 (359.2-840.5) | 23.9 (21.3-25.0) | 404.1 | **17.49x** (15.73-33.93x, 7/7) | 16.34x (13.91-19.74x, 7/7) | 200 answers, 190 True | 45.4 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_cond | 40 | 202,475.1 | 231,384.8 (187,072.0-275,419.6) | 3,573.1 (2,967.8-6,235.7) | 211,865.0 | **62.40x** (38.98-92.80x, 7/7) | 56.67x (30.79-83.88x, 7/7) | 40 answers, 38 True | 45.6 / 71.1 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_cond | 200 | 1,857.1 | 1,913.3 (1,804.8-2,202.8) | 50.8 (44.4-65.4) | 1,921.6 | **37.43x** (33.69-42.54x, 7/7) | 37.84x (30.43-44.18x, 7/7) | 200 answers, 189 True | 47.1 / 47.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_row_present: native rows+dense 104,939+0 -> 104,939+0
- te_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

#### stream-cond (stream + if/case template tree, survey rank 12 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_cond | 200 | 630.5 | 691.4 (598.8-835.4) | 89.8 (78.6-114.1) | 673.0 | **7.57x** (7.00-8.75x, 7/7) | 7.08x (5.20-7.64x, 7/7) | 7,268 | 44.9 / 45.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_left_out | 200 | 162.3 | 195.9 (175.8-208.4) | 44.3 (36.6-59.2) | 195.5 | **4.51x** (3.52-5.33x, 7/7) | 3.84x (2.85-4.18x, 7/7) | 700 | 103.2 / 103.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_cond | 200 | 153.1 | 167.9 (160.7-223.7) | 36.4 (30.6-50.7) | 184.4 | **4.89x** (3.20-7.05x, 7/7) | 4.48x (2.90-5.30x, 7/7) | 1,620 | 44.9 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_cond | 40 | 71,129.7 | 76,682.1 (73,867.1-78,446.9) | 10,045.2 (9,078.5-10,759.9) | 74,329.8 | **7.40x** (7.16-8.64x, 7/7) | 7.08x (6.70-8.17x, 7/7) | 173,693 | 49.5 / 56.7 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_cond | 200 | 824.6 | 860.4 (825.3-963.2) | 109.6 (99.9-143.6) | 853.4 | **7.92x** (6.71-8.53x, 7/7) | 7.46x (6.55-8.43x, 7/7) | 9,643 | 46.5 / 46.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_left_out: native rows+dense 104,995+0 -> 104,995+0
- te_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

#### collapse-cond (collapse + template tree with a boolean leaf; not a survey family (0 occurrences), the same compiler (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_cond | 200 | 1,330.7 | 1,376.1 (1,240.9-1,653.9) | 48.3 (43.4-50.4) | 1,404.3 | **30.90x** (25.41-34.79x, 7/7) | 29.47x (24.53-31.67x, 7/7) | 200 tuples, 7,268 items | 44.9 / 45.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_cond | 200 | 304.7 | 335.4 (302.6-507.7) | 27.3 (24.0-44.4) | 349.2 | **12.33x** (7.56-17.78x, 7/7) | 11.27x (7.52-12.70x, 7/7) | 200 tuples, 1,620 items | 44.9 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_cond | 40 | 160,033.4 | 170,605.8 (154,162.2-221,151.2) | 4,071.9 (3,293.9-4,496.8) | 167,401.4 | **40.32x** (37.50-58.22x, 7/7) | 39.73x (34.05-65.71x, 7/7) | 40 tuples, 173,693 items | 45.1 / 63.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_cond | 200 | 1,807.8 | 1,917.1 (1,744.5-3,099.7) | 64.6 (56.3-128.4) | 1,811.5 | **29.85x** (18.89-45.99x, 7/7) | 28.32x (18.14-38.70x, 7/7) | 200 tuples, 9,643 items | 46.6 / 47.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- te_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

#### Checks

- every site: the same answers per batch (and answer items) in all 5 modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call


Reading. On the probe-style sites the tree shapes behave like fnspec 2's filter shapes (collapse(once) and existence
tests cut at the first surviving row: 16x-102x against CeTTa; stream and collapse forms pay for every row: 4.5x-40x).
On real code: the ledger.metta:146 case (stream-cond) 3.8x and the hypothesis_mining.metta:83 boolean leaf (once-cond)
2.7x against CeTTa. core.metta:258 (rs_sk_summary_memo, 0.95x) and skeleton.metta:643 (rs_sk_row_present, 0.89x)
gain nothing: both modes spend about 0.9 and 2 ms a query in the matcher itself (fn-hosted, which is CeTTa's
matcher behind a callback and nothing else, costs as much), against about 40 us for the other real sites. Both
patterns bind the first and the third argument with a variable between them, and the time scales with the rows that
share the third argument (5,000 per value in sk_summary, 10,000 in sk_row) rather than the first (about 20): an
inference from the timings, not traced in CeTTa. Improving it is matcher/index work, outside this order (no PathMap
index).

## 3. Bag tests (make fnspec-difftest, part of make test)

Every fnspec site (the 47 of fnspec 1 and 2 and the 20 new ones; 67), with the method of fnspec 1 and 2: the small
spaces of tests/fnspec_cases.py (new kinds hm_seen, sk_summary, sk_row, sk_member: several rows per key in an order
where the first row is not the only one, rows on every branch of the tree, case patterns with 0.0, -0.0, 0, 0e0 and
a string where 0.0 is expected, no case branch matching, all rows of a key taking (empty); ";; expect: mixed" cases
with variables in rows, numbers inside compared expressions, int/float mixes and bignums, where the residual is
allowed) and the benchmark KB with the site's queries. In each case: fn-hosted, fn-spec (callback), fn-fallback
(forced INVALID) and, new, fn-rows (the match-rows hook) against fn-generic (CeTTa evaluating the function), each in
its own engine, bag for bag; then, on the first case, in both compiled paths (fn-spec and fn-rows): the mutation
sequence (second equation added/removed, redefinition and restore, builtin shadowed/unshadowed, declaration
added/removed, row-not-normal; for imported functions the module-space steps and a second module), recursion at
depth 1, 10, 100 and 1000, the profile-he test for once plans, and the language-petta forced mismatch for
existence-test plans; and the eq-probe check in the site's own engine.

Result (evidence/fnspec3-2026-11-24/difftest, at 0a3a591; env.txt): 67 sites, 3673 checks, 0 failures, 8 skips (the named-space language-petta tests). Of them: 1624 mode-vs-reference (fn-hosted, fn-spec, fn-fallback, fn-rows: 406 each), 1394 mutation steps (697 per path), 536 recursion (268 per path), 68 profile-he, 28 language-petta, 23 eq-probe. The 20 new sites account for 1100 checks.

Notes from the new cases (all as CeTTa does, now pinned by tests): a stream whose every row takes an (empty) branch
of a tree gives one Empty per row, as fnspec's stream-filter; a case with no matching branch gives nothing; case
patterns match numbers as CeTTa's case does (0.0 against -0.0, 0, 0e0 and "w": the bag tests compare the compiled
bag with CeTTa's on each); =alpha or == on an expression with a number inside, and int against float, take the
residual (fs_compare's kinds, unchanged since fnspec), so such rows are in the mixed cases.


## 4. Item 3: match-rows row hook (upstream/cetta/0004)

### 4.1 What 0004 adds, and how fnspec uses it

0004 (upstream/cetta/0004-match-rows-row-hooks-compiled-code-inside-the-matcher.patch, PR_DESCRIPTION-0004.md;
4 files, +296 lines) gives 0003's match-rows a five-argument form,

    (match-rows <space> <pattern> <row> (<hook> <arg> ...) <residual>)

that runs the same conjunction matcher and hands each binding row (its Bindings and <row> as the call
instantiated it) to a host hook registered under <hook> (cetta_embed_native_rows_hook_register): enter(args) once
per call (1 run, 0 residual; may ask for the first-row cut), query() (optional: the host's own compiled patterns and
seed Bindings, run in place of <pattern>), row(...) per row in the matcher's order (1 keep, 0 drop, 2 none, -1
abort), leave(kept, dropped, aborted) for the call's answers (or 0: residual). When enter or leave answers 0,
match-rows evaluates <residual> in place of the call. Rows, per-row work and leave's answers live in a scratch arena;
match-rows copies the answers into the evaluation. It applies to a9b3a811 after 0001-0003
(git am of the four files on a fresh clone: evidence/fnspec3-2026-11-24/patch-apply.txt), and build_cetta.sh
builds it (build tree head = 0004).

For every plan, fs_open registers a hook under a unique name and loads (tools/fnspecc.py rows_def)

    (= (cpk-rows:f A1 ... An) (match-rows S P (CV1 ... CVk T1 ... Tm) (<hook> A1 ... An) (f A1 ... An)))

where P is the plan's pattern (conjunction), CVi the variables the condition compares and Tj the template atoms
(the data template, or a tree's data leaves and case patterns). The hook supplies the plan's own query (its patterns,
and the call's arguments bound to its variables as seeds, exactly what compute() passes to the matcher), so P and
the row tuple only say what match-rows would run for a hook without a query (section 4.3 for why). The generated row
function then reads each row as on the callback path. The hook is the callback path's code, split where match-rows
calls it:

| callback path (cpk-fnspec:f, run_call + compute) | hook path (cpk-rows:f, rows_enter/row/leave) |
|---|---|
| forced INVALID -> residual | the same, in enter |
| program token moved -> revalidate (one equation of f, alpha-equal; no declaration; builtins as a fresh engine; no catch-all; once and == probes) | the same function (revalidate), in enter |
| arguments: normal form, plain ground, else residual | the same checks, in enter |
| space argument not a space -> residual | match-rows resolves it; not a space -> leave(aborted) -> residual |
| CeTTa's matcher (space_query_conjunction) on the plan's patterns, seeds bound, in a detached arena | the same call on the same patterns and seeds (the hook's query), in match-rows' scratch arena |
| per row: the generated row function (condition kinds, -1 abort; template; per-answer normal form) | the same generated function, called by match-rows |
| collapse(once)/existence: stop at the first kept row | the same cut (first_only), inside match-rows' row loop |
| wrapper: stream / collapse tuple / he's True/False / Empty per dropped row; answers copied into handles | the same, in leave; answers copied into the evaluation by match-rows |
| residual: CeTTa evaluates (f args) | match-rows evaluates (f args) in place of the call |

Nothing is skipped: the hook path's counters (calls, spec, residual, invalid, arg_rejects, row_aborts) are the
callback path's, and every bag test (mutation, forced INVALID, recursion, profile, language) runs in both paths
(section 3).

### 4.2 Hook vs callback, every site (RUNS=7 REPEAT=5, interleaved; evidence/fnspec3-2026-11-24/hook-bench, rows-hook.md)

bench/run_fnspec.sh with MODES="fn-spec fn-rows" on all 60 benchmark sites (fnspec 1: 19, fnspec 2: 21, fnspec 3: 20), at the final commit; bench/summarize_rows_hook.py; make fnspec3-recompute regenerates rows-hook.md from the raw runs.

fn-spec: the compiled function as an embedding callback (cpk-fnspec:f); fn-rows: the same generated code inside match-rows through the 0004 row hook (cpk-rows:f). Both run the same guards (program token / definition check, argument checks), the same row function (condition, template, per-answer normal-form check) and the same first-row cut. Paired: run i of fn-spec over run i of fn-rows; > 1 means the hook is faster.

#### evidence/fnspec3-2026-11-24/hook-bench

| site | shape | queries | fn-spec us | **fn-rows us** | **fn-rows vs fn-spec, paired** | answers per batch | peak RSS fn-rows / fn-spec MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|
| gv_collapse_cond | collapse-cond | 200 | 44.0 (41.1-52.2) | **39.7** (34.1-43.2) | **1.11x** (1.02-1.31x, 7/7) | 200 | 44.9 / 45.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_collapse_data | collapse-data | 200 | 52.6 (49.8-61.9) | **45.7** (42.3-68.5) | **1.22x** (0.77-1.36x, 5/7) | 200 | 44.8 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_collapse_filter | collapse-filter | 200 | 56.8 (50.6-60.2) | **49.1** (44.0-55.4) | **1.14x** (0.94-1.37x, 6/7) | 200 | 45.3 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_cond | exist-cond | 200 | 38.7 (33.7-41.2) | **33.8** (29.5-74.8) | **1.03x** (0.55-1.31x, 5/7) | 200 | 45.4 / 45.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_data | exist-data | 200 | 40.2 (36.5-65.7) | **37.7** (31.9-68.6) | **1.06x** (0.69-1.89x, 4/7) | 200 | 44.9 / 44.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_filter | exist-filter | 200 | 38.9 (37.0-69.8) | **37.0** (35.2-74.5) | **1.09x** (0.50-1.81x, 5/7) | 200 | 45.2 / 45.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_cond | once-cond | 200 | 41.4 (39.4-47.7) | **36.3** (34.5-77.7) | **1.12x** (0.52-1.27x, 6/7) | 200 | 45.5 / 45.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_data | once-data | 200 | 42.3 (36.4-47.5) | **35.9** (32.6-56.1) | **1.13x** (0.79-1.26x, 6/7) | 200 | 44.8 / 44.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_filter | once-filter | 200 | 41.9 (39.1-63.7) | **35.6** (32.1-38.1) | **1.13x** (1.10-1.93x, 7/7) | 200 | 44.3 / 44.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_cond | stream-cond | 200 | 88.5 (80.9-117.1) | **79.0** (73.0-129.2) | **1.14x** (0.91-1.21x, 5/7) | 7,268 | 44.9 / 45.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_data | stream-data | 200 | 80.5 (70.3-132.3) | **67.7** (58.6-131.4) | **1.14x** (0.60-1.48x, 6/7) | 7,377 | 44.4 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_filter | stream-filter | 200 | 126.5 (68.0-160.0) | **75.4** (66.4-124.5) | **1.27x** (0.89-2.27x, 6/7) | 7,268 | 44.2 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_base_rate_rows | collapse-filter | 200 | 37.7 (36.6-79.2) | **28.4** (26.3-56.1) | **1.36x** (0.73-2.33x, 6/7) | 200 | 90.9 / 91.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_compound_connective | exist-data | 200 | 23.2 (22.0-40.6) | **14.5** (12.6-24.1) | **1.75x** (0.92-2.34x, 6/7) | 200 | 22.0 / 21.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_given_rule | exist-data | 200 | 24.1 (20.3-41.6) | **14.7** (13.9-28.7) | **1.62x** (0.95-1.71x, 6/7) | 200 | 76.9 / 76.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_hm_recurs | once-cond | 200 | 41.5 (39.9-46.4) | **32.0** (25.2-42.5) | **1.30x** (1.09-1.65x, 7/7) | 200 | 63.1 / 63.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_live_hypotheses | collapse-data | 200 | 216.0 (192.2-242.2) | **174.4** (139.7-199.4) | **1.24x** (1.09-1.73x, 7/7) | 200 | 259.1 / 259.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_mil_run | stream-data | 200 | 25.1 (22.1-26.2) | **21.5** (18.4-33.7) | **1.10x** (0.71-1.33x, 4/7) | 201 | 42.2 / 42.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_left_out | stream-cond | 200 | 47.1 (39.6-73.0) | **31.7** (29.5-33.2) | **1.47x** (1.33-2.25x, 7/7) | 700 | 102.8 / 103.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_needed | exist-filter | 200 | 61.1 (46.8-76.3) | **45.4** (34.5-85.5) | **1.20x** (0.58-2.18x, 4/7) | 200 | 50.4 / 50.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_needed_depth | once-filter | 200 | 52.8 (50.0-62.5) | **42.8** (37.4-85.2) | **1.32x** (0.61-1.46x, 5/7) | 200 | 49.9 / 49.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_row_present | exist-cond | 200 | 2,370.6 (2,059.3-3,323.0) | **2,560.7** (2,037.7-3,209.6) | **1.03x** (0.64-1.30x, 5/7) | 200 | 72.1 / 72.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_summary_memo | once-cond | 200 | 1,115.5 (984.7-1,489.0) | **1,024.9** (943.6-1,148.2) | **1.09x** (0.86-1.58x, 6/7) | 200 | 39.7 / 39.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_stored_proof_tvs | once-data | 200 | 22.1 (21.0-37.2) | **14.6** (12.7-30.2) | **1.57x** (0.80-2.02x, 6/7) | 200 | 50.8 / 50.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_cond | collapse-cond | 200 | 27.5 (24.0-29.3) | **23.4** (19.9-29.6) | **1.19x** (0.91-1.21x, 6/7) | 200 | 45.0 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_data | collapse-data | 200 | 30.3 (25.8-32.7) | **24.8** (21.5-26.8) | **1.21x** (1.03-1.35x, 7/7) | 200 | 44.6 / 44.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_filter | collapse-filter | 200 | 30.1 (24.4-56.7) | **24.2** (21.9-25.8) | **1.28x** (1.03-2.27x, 7/7) | 200 | 45.2 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_cond | exist-cond | 200 | 25.1 (21.0-28.3) | **20.6** (17.9-39.0) | **1.19x** (0.54-1.52x, 5/7) | 200 | 45.3 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_data | exist-data | 200 | 24.1 (23.5-45.1) | **19.6** (17.1-33.8) | **1.34x** (0.79-2.30x, 6/7) | 200 | 44.1 / 44.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_filter | exist-filter | 200 | 22.1 (20.9-24.0) | **17.8** (16.3-35.9) | **1.23x** (0.66-1.46x, 6/7) | 200 | 45.3 / 45.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_cond | once-cond | 200 | 27.1 (25.6-54.3) | **20.9** (19.0-43.5) | **1.27x** (0.79-2.85x, 6/7) | 200 | 44.2 / 44.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_data | once-data | 200 | 27.7 (24.0-53.1) | **19.8** (17.5-37.8) | **1.37x** (0.79-2.72x, 6/7) | 200 | 44.0 / 44.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_filter | once-filter | 200 | 24.1 (22.8-31.2) | **20.4** (18.2-21.2) | **1.26x** (1.12-1.72x, 7/7) | 200 | 44.4 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_cond | stream-cond | 200 | 37.9 (31.5-55.8) | **32.5** (27.7-34.0) | **1.18x** (0.98-1.91x, 6/7) | 1,620 | 44.8 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_data | stream-data | 200 | 31.2 (28.7-35.5) | **27.6** (25.5-53.3) | **1.18x** (0.58-1.39x, 5/7) | 1,653 | 44.2 / 44.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_filter | stream-filter | 200 | 30.6 (29.3-33.8) | **27.2** (25.2-31.2) | **1.12x** (1.01-1.25x, 7/7) | 1,620 | 44.9 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| tf_collapse_cond | collapse-cond | 40 | 3,733.2 (2,647.0-4,258.0) | **3,239.3** (3,197.3-3,535.2) | **1.15x** (0.75-1.29x, 4/7) | 40 | 45.1 / 45.3 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_collapse_data | collapse-data | 40 | 5,980.9 (5,148.0-6,339.0) | **5,443.4** (4,724.9-6,003.7) | **1.06x** (0.98-1.15x, 6/7) | 40 | 46.4 / 46.4 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_collapse_filter | collapse-filter | 40 | 6,691.3 (5,361.6-7,302.8) | **5,720.9** (4,738.7-6,400.5) | **1.09x** (0.89-1.46x, 5/7) | 40 | 47.1 / 47.1 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_cond | exist-cond | 40 | 2,666.7 (2,222.3-2,879.6) | **2,712.5** (2,212.1-4,149.4) | **1.04x** (0.66-1.24x, 4/7) | 40 | 45.7 / 45.8 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_data | exist-data | 40 | 2,713.1 (2,198.9-3,257.8) | **2,668.9** (2,194.0-3,265.6) | **1.10x** (0.67-1.25x, 4/7) | 40 | 45.4 / 45.3 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_filter | exist-filter | 40 | 3,053.9 (2,218.8-4,090.1) | **2,592.9** (2,330.9-4,263.8) | **0.86x** (0.72-1.65x, 3/7) | 40 | 45.7 / 45.5 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_cond | once-cond | 40 | 2,663.2 (2,300.6-2,829.0) | **2,588.8** (2,115.5-3,494.2) | **1.03x** (0.66-1.34x, 5/7) | 40 | 46.4 / 46.1 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_data | once-data | 40 | 2,503.5 (2,125.1-3,608.1) | **2,731.9** (2,644.2-6,291.0) | **0.83x** (0.57-0.93x, 0/7) | 40 | 45.0 / 45.2 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_filter | once-filter | 40 | 2,737.3 (2,160.0-2,810.1) | **2,756.8** (2,127.7-3,235.9) | **1.01x** (0.69-1.20x, 5/7) | 40 | 45.1 / 45.0 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_cond | stream-cond | 40 | 9,975.8 (8,625.2-13,726.5) | **8,788.9** (8,364.7-11,708.9) | **1.13x** (1.03-1.19x, 7/7) | 173,693 | 49.5 / 49.7 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_data | stream-data | 40 | 8,973.4 (8,190.6-12,704.7) | **9,516.3** (7,771.2-12,672.7) | **1.00x** (0.83-1.19x, 4/7) | 173,693 | 48.7 / 49.0 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_filter | stream-filter | 40 | 8,979.6 (8,156.7-11,563.1) | **8,683.7** (7,170.4-11,554.0) | **1.03x** (0.95-1.30x, 5/7) | 173,693 | 48.3 / 48.8 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| vi_collapse_cond | collapse-cond | 200 | 61.2 (53.1-68.5) | **58.7** (52.2-71.4) | **1.06x** (0.82-1.19x, 5/7) | 200 | 46.6 / 46.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_collapse_data | collapse-data | 200 | 74.7 (66.2-90.0) | **67.2** (56.4-78.2) | **1.13x** (0.87-1.60x, 4/7) | 200 | 46.2 / 46.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_collapse_filter | collapse-filter | 200 | 84.0 (81.4-85.4) | **75.0** (58.2-77.8) | **1.12x** (1.09-1.43x, 7/7) | 200 | 46.4 / 46.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_cond | exist-cond | 200 | 53.9 (50.9-60.7) | **45.1** (37.7-52.3) | **1.20x** (1.04-1.37x, 7/7) | 200 | 47.2 / 47.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_data | exist-data | 200 | 53.8 (43.4-62.9) | **46.8** (39.0-59.6) | **1.14x** (0.96-1.38x, 5/7) | 200 | 46.4 / 46.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_filter | exist-filter | 200 | 51.1 (45.1-57.5) | **44.2** (40.5-54.5) | **1.12x** (0.84-1.32x, 5/7) | 200 | 46.9 / 47.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_cond | once-cond | 200 | 50.3 (48.0-60.8) | **45.7** (42.1-57.7) | **1.07x** (0.90-1.30x, 5/7) | 200 | 46.8 / 46.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_data | once-data | 200 | 50.8 (46.5-98.4) | **52.7** (43.5-64.0) | **1.02x** (0.92-1.87x, 4/7) | 200 | 46.5 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_filter | once-filter | 200 | 51.8 (50.2-58.3) | **46.6** (42.1-58.5) | **1.08x** (0.93-1.29x, 4/7) | 200 | 46.7 / 46.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_cond | stream-cond | 200 | 119.6 (101.9-181.9) | **102.0** (91.1-117.9) | **1.22x** (0.86-1.80x, 5/7) | 9,643 | 46.4 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_data | stream-data | 200 | 101.6 (98.8-108.4) | **93.6** (79.2-111.8) | **1.06x** (0.88-1.37x, 6/7) | 9,835 | 46.2 / 46.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_filter | stream-filter | 200 | 107.4 (98.4-132.1) | **93.2** (84.2-118.5) | **1.23x** (1.00-1.27x, 6/7) | 9,643 | 46.5 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |

#### Over all sites

- 60 sites; paired fn-rows vs fn-spec, median over sites 1.14x, range 0.83x-1.75x; faster on 58 of 60 sites (median ratio > 1)

#### Checks

- every site: fn-spec and fn-rows give the same answers per batch (answer items, True answers, non-empty tuples); both specialised every timed call; no bad answers


### 4.3 Revision 1 and why the hook now supplies its own query

The first revision of 0004 ran the matcher on <pattern> as the call instantiated it, and handed the hook the row
under those bindings. On the first six sites of the part 2 benchmark (evidence/fnspec3-2026-11-24/bench, fn-rows
column, revision 1) the hook was slower than the callback: 0.65x-0.97x paired. Timing inside match-rows (a
diagnostic build outside the repository: the time from enter to leave against the callback's compute()) showed the
extra time inside the matcher and row loop, not in the call: for gv_collapse_cond about 50 us against 31 us per
call. The difference is the query: the callback runs the plan's own patterns with the arguments as seed bindings;
the equation instantiated a copy of the pattern with fresh variables, and CeTTa's matcher and bindings_apply cost
more on those (spot checks, same binary, 3 interleaved processes each, us per query, total/inside):

| site | callback | hook, call's pattern (rev 1) | hook, own query (rev 2) |
|---|---|---|---|
| gv_collapse_cond | 47-96 / 28-55 | 66-116 / 47-85 | 44-47 / 28-34 |
| rs_sk_needed | 53-114 / 41-77 | 107-126 / 89-119 | 43-56 / 40-48 |
| tf_collapse_data | 7,137-8,346 | 10,338-11,408 | 6,079-6,589 (one run 11,200) |
| rs_given_rule | 24-63 / 2-9 | 14-25 / 1-2 | 15-16 / 1 |

Revision 2 (0004 as committed) adds an optional query to the hook: the host supplies its compiled patterns and
seed Bindings over its own variables, and match-rows runs the same conjunction matcher on them; the rows and the
per-row work live in a scratch arena and only the call's answers are copied out (as the callback copies its answers
into handles). The generated row function then reads each row exactly as compute() does. These spot checks are
diagnostics (a nice'd process beside the running benchmark, not the method); the measurement is section 4.2.

## 5. Acceptance (the order's items)

1. Part 1. The forced-mismatch test takes the residual: 28/28 language-petta tests (14 sites x callback and hook),
   spec=0, residual=queries, eq_mismatch=yes, the PeTTa engine's own answers. The matching engine still
   specialises: every exist site in its he engine specialises every non-mixed call (fn-spec and fn-rows rows of the
   bag tests, and the benchmark's path column), 23/23 eq-probe checks.
2. Part 2. Bag tests 0 failures on every new shape and site (20 sites; 67 in all); forced INVALID (fn-fallback) took
   the residual on every call, every redefinition/mutation step took the expected route, in both paths; tables
   recompute from the raw runs (make fnspec-recompute: "recomputed tables match" for fnspec 1, 2 and 3's summary.md and for rows-hook.md).
3. Part 3. 0004 applies to a9b3a811 after 0001-0003 (evidence/fnspec3-2026-11-24/patch-apply.txt; build_cetta.sh
   builds it); a paired table, hook vs callback, for every one of the 60 benchmark sites (section 4.2; make
   fnspec3-recompute); bag tests 0 failures in the hook path (section 3).
4. make test: passes (EXIT=0 at the final tree; evidence/fnspec3-2026-11-24/make-test.log: hashes, M1 test, HE diff, cross-space refusal, regenerated certificate, kernel difftest, probe difftest, fnspec difftest 67 sites 0 failed). Answers at earlier sites unchanged: the 47 fnspec 1/2 sites' bag tests (same cases,
   same reference) pass in all four earlier modes, the probes' difftest is in make test, the generated C of those
   sites differs only in the template macro and three plan fields (section 2.2), and the probe-style stream-filter
   sites give the probes' recorded answers per batch in the hook benchmark (known-answer canary: te 1,620, gv 7,268,
   tf 173,693, vi 9,643).

## 6. Limits

- One machine (pop-os, shared, load about 3-4), 7 processes per mode; the per-site medians move by 10-40% between
  runs on the sub-100 us sites, which is why every ratio is paired run by run and counted (k/n).
- The part 2 benchmark ran at 96b3fa7 (0004 revision 1); its fn-spec, fn-generic, fn-hosted and fn-fallback numbers
  are of code identical to the final callback path except the row macros' NULL test (removed in 0a3a591). Its
  fn-rows column measured revision 1 of the hook and is superseded by section 4 (all sites, final revision).
- Real-code sites run real PeTTaChainer function bodies (or the collapse(once)/match subexpression a function
  evaluates) on generated rows with each pattern's shape, under CeTTa's he language, profile extended where once is
  used; PeTTa's own runtime is not compared. 3 of the 10 survey occurrences of the family are refused (calls, a
  non-Bool condition leaf, nested match/let, unify), and rank 10 (match-count) is not compiled.
- The language-petta forced-mismatch test runs on the 14 space-argument existence-test sites; the 4 named-space
  existence sites are skipped (their bind! token does not resolve through quote in a PeTTa engine); the probe itself
  is the same code for both.
- The hook is a process-wide registry without locking (registration from one thread); 0004 is a file in this
  repository, not offered to Zar (stop-line).
- Still no bounded matcher: the first-row cut stops the row loop, not the matcher (no PathMap index, per the order).
