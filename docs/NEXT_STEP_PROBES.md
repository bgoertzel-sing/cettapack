# Next step: which part of the plan beats CeTTa's own matcher? Three probes

Task t-2026-11-18-cettapack-next-step-probe (andy, Work msg 760). Branch hive/t-2026-11-18-cettapack-next-step-probe
from main 64fa80d. Evidence: evidence/probes-2026-11-18/ (raw JSONL per site, commands, env, summaries, bag tests).

Question: since a single compiled match does not beat CeTTa's conjunction matcher behind the same callable
(hosted-generic, docs/BENCHMARK.md), which of fusing several matches, specialising whole functions, or a direct
entry point to CeTTa's matcher gives a measured gain over hosted-generic?

**Answer, in one line:** specialising a whole function does (8.8-15.7x over the same function with its match
hosted on CeTTa's matcher, all four sites, 7/7 runs each); the direct entry point does for a bare match
(1.36-2.40x over hosted-generic, 7/7 runs each); fusion does not (the fused kernel is 0.75-0.92x of hosted-generic,
and CeTTa already runs nested matches at about the cost of the conjunction).

## Method

Same host and settings as the M1 numbers: pop-os, i7-1165G7, 8 threads, 15 GiB, Ubuntu 22.04, gcc 11.4; CeTTa
a9b3a811 + 0001 + 0002 + 0003, BUILD=mork; the M1 KB (bench/gen_kb.py --seed 20261106, 128,576 atoms, sha256
8097f2f4...5439f5c) and the same seeds per site (200, 200, 40, 200). RUNS=7 processes per mode, REPEAT=5 batches
per process, the process's median batch, modes interleaved run by run (bench/run_probes.sh); per-query time is the
median over the 7 processes with the min-max range; "paired" divides run i of the reference by run i of the mode
(> 1: the mode is faster). Shared host, load average 2.3-3.1 during the run (env.txt).

One change of method, applied to every mode: every query atom is built before the timed loop
(build/cettapack-probe-<site>, src/cpk_probe.c), and every mode makes one untimed warm-up call. The docs/BENCHMARK.md
host built the atoms inside the loop, and its generic mode parsed the whole match text per query while the callable
modes parsed only the seed. The old method was rerun in the same session, interleaved (modes m-generic,
m-hosted-generic, build/cettapack-host-<site>); see "Correction" below.

Route (as in docs/ROUTE_CHECK.md, stats-off facts read from CeTTa's Space before and after the timed batches):
every mode on every site ran on CeTTa's native backend with native rows. The modes on CeTTa's matcher held 82,177
rows + 46,399 dense facts (variant_interactor: 112,164 + 16,412) throughout, the same within a site; the kernel,
fallback and fn-kernel modes held 128,576 rows + 0 (the kernel's index build moves every fact to rows, as in the
route check). No mode ran on a PathMap-only route; runtime counters were not taken (no stats build).

## Correctness (bag tests)

build/cettapack-probe-<site> difftest (evidence/probes-2026-11-18/difftest-<site>.txt): for every seed, every
mode's result bag against generic CeTTa's (evaluated match for the match modes; CeTTa evaluating the MeTTa function
for the fn modes), each mode in its own engine; on six small spaces (M1-numbers' four: duplicates, co-reference, equal
values in distinct variables, dangling joins, a 70-byte interned symbol; plus d5 and d6, where the filter constant
occurs and every row of a seed is filtered), every symbol of the space as a seed plus an absent one; then every seed of
the site's benchmark query file on the benchmark KB. Path checks: every kernel call on the kernel path, every fallback
and fn-fallback call on the residual, every fn-spec call specialised. **All modes, all four sites: 0 failures**
(e.g. M1's site on the KB: 200/200 bags per mode). This also checks hosted-generic bag by bag, which docs/BENCHMARK.md
had checked only by counts.

The bag tests caught one semantic detail the specialised function must reproduce: when every row of a call takes the
(empty) branch, CeTTa's result at the embedding boundary is one Empty atom per row; when any row gives a value, the
Empty results are dropped (d5 rs3, d6 rs1/rs2/rs4). The specialised function does the same; without it, 4 of 25 small-
space seeds differed.

## Probe 1: direct entry point (upstream/cetta/0003, match-rows)

0003 adds one builtin to CeTTa's evaluator (35 lines, src/eval.c and src/symbol.h; PR_DESCRIPTION-0003.md):
(match-rows <space> <pattern> <row>) runs space_query_conjunction on the pattern (one pattern or a flat
(, p1 ... pn)) and returns, for each binding row, <row> with the bindings applied, as data: no template is evaluated.
Called from MeTTa as (match-rows S (, (target rs5 $gene) (expressed $gene $tissue)) ($gene $tissue)).

| site | generic (match) us | hosted-generic us | **direct (match-rows) us** | kernel us | fallback us | direct vs hosted-generic, paired | peak RSS direct / hosted-generic MiB |
|---|---|---|---|---|---|---|---|
| target_expressed (M1) | 31.1 (28.5-35.4) | 29.4 (26.9-34.9) | **12.2** (11.2-13.4) | 36.4 (32.2-40.5) | 56.9 (51.5-65.3) | **2.40x** (2.20-2.80x, 7/7) | 42.3 / 42.4 |
| gene_variants | 107.2 (102.4-205.1) | 66.6 (60.2-73.7) | **35.9** (33.8-41.9) | 75.6 (72.1-143.8) | 150.9 (148.8-175.4) | **1.77x** (1.52-2.18x, 7/7) | 42.3 / 42.6 |
| tissue_fanout | 16,309.8 (15,227.8-16,940.6) | 8,358.4 (7,513.8-9,468.2) | **6,069.0** (5,235.1-6,550.8) | 8,984.9 (8,144.2-10,102.2) | 19,200.1 (18,597.9-19,879.3) | **1.36x** (1.22-1.80x, 7/7) | 46.7 / 46.8 |
| variant_interactor | 171.4 (159.9-183.9) | 86.4 (82.7-95.1) | **62.7** (57.6-72.3) | 112.9 (101.1-115.5) | 213.3 (200.6-241.9) | **1.40x** (1.16-1.52x, 7/7) | 44.1 / 44.2 |

Fallback for this probe: match-rows is CeTTa's own primitive and needs no guard; a site that cannot use it (template
not data, see below) stays on match, i.e. the generic column. The fallback column is the site callable's residual,
as in docs/BENCHMARK.md.

direct and hosted-generic call the same matcher (space_query_conjunction) on the same storage; they differ only in
what happens to the rows. hosted-generic instantiates the template, deep-copies the answers into engine-owned handles
and hands them back through the embedding callback; match-rows adds each instantiated row to the evaluator's outcome
set. So 17 us of hosted-generic's 29 us on M1's site (31 of 67 us on gene_variants) is the callback boundary and
answer hand-back, not matching. That is an inference from the two modes' code paths, not a profile.

Condition for equal answers: match evaluates each instantiated template; match-rows does not. They agree when the
instantiated rows are in normal form (no equation or grounded operation applies to them), as here (bag tests). A
compiler may use match-rows for a match only where it can establish that (e.g. a template of variables bound to
symbols that head no equation); otherwise the site stays on match.

## Probe 2: fusing several matches

The same matches written one after another, (match S (target rs5 $gene) (match S (expressed $gene $tissue)
($gene $tissue))) ("nested"; for variant_interactor three levels), run by CeTTa, against one fused kernel: the
compiled kernel of the fused conjunction (the M1/M1-numbers kernel: guard, PathMap narrowing per factor, native
rematch). The fusion step itself (nested -> conjunction) was done by hand for the probe (packs/<site>/site.metta is
already the conjunction); bag tests check the nested form against the conjunction and the kernel.

| site | nested (CeTTa, one after another) us | generic conjunction us | hosted-generic us | **fused kernel** us | fallback us | fused kernel vs hosted-generic, paired | nested vs hosted-generic, paired | peak RSS fused kernel / hosted-generic MiB |
|---|---|---|---|---|---|---|---|---|
| target_expressed | 32.3 (29.6-35.0) | 31.1 | 29.4 | **36.4** (32.2-40.5) | 56.9 | **0.87x** (0.73-1.00x, 0/7) | 0.95x (0/7) | 566.6 / 42.4 |
| gene_variants | 114.8 (107.0-124.6) | 107.2 | 66.6 | **75.6** (72.1-143.8) | 150.9 | **0.84x** (0.42-0.98x, 0/7) | 0.57x (0/7) | 566.2 / 42.6 |
| tissue_fanout | 15,495.0 (14,646.6-16,197.8) | 16,309.8 | 8,358.4 | **8,984.9** (8,144.2-10,102.2) | 19,200.1 | **0.92x** (0.83-1.06x, 2/7) | 0.52x (0/7) | 571.0 / 46.8 |
| variant_interactor (3 matches) | 195.3 (172.9-231.8) | 171.4 | 86.4 | **112.9** (101.1-115.5) | 213.3 | **0.75x** (0.72-0.92x, 0/7) | 0.46x (0/7) | 819.2 / 44.2 |

The fused kernel beats the matches run one after another by CeTTa on three sites (1.5-1.7x vs nested; not on M1's
site, 0.89x) but loses to hosted-generic on all four. CeTTa runs the nested form within 4-14% of its own conjunction
(and 5% faster on tissue_fanout): its evaluator already fuses nested match chains (src/eval.c, "match (with nested-match fusion + join
reordering)"). Everything the fused kernel gains over nested comes from hosting the call, which hosted-generic gets
without the kernel and without the 520-775 MiB index. Fusion has nothing left to win at these sites.

## Probe 3: specialising a whole function

One small MeTTa function per site: the site's match with a filter on the last template variable,

    (= (cpk-fn $space $seed) (match $space (, (target $seed $gene) (expressed $gene $tissue))
                               (if (== $tissue tissue0) (empty) ($gene $tissue))))

(the other sites: their own conjunction and template; tissue_fanout's filter tests $gene, which is never tissue0, so
it filters nothing there). Modes:

- fn-generic: CeTTa evaluates the function.
- fn-hosted (the hosted-generic baseline for a function): the same function with its match replaced by the
  hosted-generic site callable, (let ($gene $tissue) (cpk-site-target-expressed $space $seed) (if ...)): what M1's
  approach gives today with CeTTa's matcher; the rest of the body evaluated by CeTTa.
- fn-kernel: the same with the compiled kernel as the site callable.
- fn-spec: the whole function as one callable (src/cpk_probe.c, fn_compute): guard (the compiled body is the body of
  cpk-fn in &self, checked by structural comparison at load; the space argument is a space; the seed a symbol), CeTTa's
  conjunction matcher (space_query_conjunction, no PathMap, no index), the filter in C (atom_eq on the bound value;
  any non-symbol value aborts to the residual), the template by bindings_apply, CeTTa's Empty rule, one answer copy.
- fn-fallback: fn-spec with its guard forced INVALID: the residual evaluates the MeTTa function.

| site | fn-generic us | **fn-hosted (hosted-generic)** us | fn-kernel us | **fn-spec** us | fn-fallback us | fn-spec vs fn-hosted, paired | fn-spec vs fn-generic | peak RSS fn-spec / fn-hosted MiB |
|---|---|---|---|---|---|---|---|---|
| target_expressed | 231.4 (217.8-241.3) | 257.5 (235.5-281.8) | 259.5 | **28.4** (27.0-55.3) | 266.2 | **8.77x** (5.00-9.93x, 7/7) | 8.1x | 42.4 / 43.0 |
| gene_variants | 952.5 (901.3-1,039.5) | 1,061.3 (976.6-1,106.7) | 1,040.9 | **66.9** (63.4-76.2) | 1,003.6 | **15.41x** (13.55-17.41x, 7/7) | 14.2x | 42.7 / 43.1 |
| tissue_fanout | 121,098.0 (115,194.0-125,671.0) | 124,041.6 (121,641.7-129,933.1) | 122,421.3 | **7,883.0** (6,999.1-8,513.1) | 122,096.4 | **15.69x** (14.94-17.74x, 7/7) | 15.4x | 46.9 / 59.4 |
| variant_interactor | 1,310.2 (1,269.4-1,448.4) | 1,385.1 (1,330.5-1,491.8) | 1,402.5 | **98.2** (94.7-114.3) | 1,403.8 | **14.07x** (11.64-15.34x, 7/7) | 13.3x | 44.3 / 45.3 |

Answers per batch: 1,620 / 7,268 / 173,693 / 9,643 in every fn mode of a site (difftest: bag by bag).

Where the time goes: hosting only the match does not help a function (fn-hosted runs at 0.90-0.98x of fn-generic's speed; the
kernel inside the function changes nothing). The function's cost is the per-answer evaluation around the match: the
if, the ==, the (empty), the template and let. fn-spec removes it, and then costs about what the bare match costs on
the same hosted path (28.4 vs 29.4 us hosted-generic on M1's site, 66.9 vs 66.6, 7,883 vs 8,358, 98.2 vs 86.4). The
gain grows with answers per query (8.8x at 8 answers per query, 14-16x at 37-4,300).

Conditions for equal answers, which a full milestone must guard rather than assume: the definition of the function is
still the compiled one (checked once at load here; a milestone needs a per-revision check of &self), if, == and empty
have their builtin meaning, the values the filter compares are symbols (checked per row here), the result rows are in
normal form (as for match-rows), and CeTTa's Empty rule above.

## Ranking and recommendation

| rank | idea | measured gain over hosted-generic (paired median, all sites) | runs faster | extra memory | work for a full milestone |
|---|---|---|---|---|---|
| 1 | **specialise whole functions** | **8.8x, 15.4x, 15.7x, 14.1x** (vs the function with its match hosted) | 28/28 | none (CeTTa's matcher, no index) | largest: a compiler for a fragment of function bodies (match + comparisons/arithmetic on bound values + if/empty + data templates) to C, guards for definitions per &self revision and for builtin meanings, residual per function, bag tests per function, CeTTa's result conventions (Empty) |
| 2 | **direct entry point (match-rows, 0003)** | **2.40x, 1.77x, 1.36x, 1.40x** (bare match) | 28/28 | none | small in code (the patch exists, 35 lines), but it is a change to CeTTa (Zar's repo), and the compiler must establish that a site's rows are data |
| 3 | fuse several matches | **none: 0.87x, 0.84x, 0.92x, 0.75x** | 2/28 | 520-775 MiB index | moderate (nested-match flattening in cpkc), but CeTTa already fuses nested matches |

Recommendation: make whole-function specialisation the next milestone, on CeTTa's own matcher (no PathMap index),
starting from the fragment probed here (one match plus filters/arithmetic on its bound values and a data template),
with per-revision definition guards and bag tests against CeTTa's evaluation of the function. Two inexpensive
additions are worth carrying with it: emit the specialised function as a CeTTa entry point rather than an embedding
callback (match-rows vs hosted-generic suggests 1.4-2.4x more on top, by avoiding the callback hand-back; measured
here only for the bare match, not for fn-spec), which is a CeTTa-side change to discuss with Zar; and drop the
PathMap kernel as the default match engine, since on every site and in every probe CeTTa's matcher is as fast or
faster and costs no index. Do not pursue fusion as a milestone at these sites.

## Correction to docs/BENCHMARK.md (hosting gain on M1's site)

In this session, with every query atom prebuilt, generic CeTTa and hosted-generic are within noise on M1's site
(31.1 vs 29.4 us; hosted-generic faster in 5/7 runs). With docs/BENCHMARK.md's method, run interleaved in the same
session, generic is 43.0 us and hosted-generic 35.3 us: about 12 us of generic's time there is the host parsing the
whole match text per query, which the callable modes do not do. So BENCHMARK.md's hosted-generic gain on M1's site
(1.34x) is mostly that parsing asymmetry; on the larger sites the gain stands (generic/hosted-generic with prebuilt
atoms: 1.61x, 1.95x, 1.98x). Kernel and fallback comparisons against hosted-generic are not affected (same
construction in both). No numbers in docs/BENCHMARK.md were changed; this note is linked from there.

## Limits

- Shared host (load average 2.3-3.1), one KB family, synthetic data, seven processes per mode.
- Probe 3 is one function shape, a filter with no arithmetic (the KB has no numeric fields); arithmetic on bound
  values would add per-answer evaluator work that specialisation removes, so the gain should not shrink, but that
  is not measured.
- fn-spec's definition guard is checked once at load, not per revision of &self; its guard cost per call is not
  separated (it is a few pointer checks).
- The route record has backend and storage facts, not runtime counters (no ENABLE_RUNTIME_STATS build this time).
- match-rows and fn-spec do not evaluate result rows; equal answers depend on rows being in normal form, which holds
  for these sites and is checked by the bag tests, not proved.
- The split of hosted-generic's time into matching and hand-back is inferred from two code paths, not profiled.
