# Shape survey: which function shapes does public MeTTa code use?

Task t-2026-11-18-cettapack-fnspec, step 2 (docs/FNSPEC_PLAN.md). Committed before any compiler code.

## Corpora (code only)

| corpus | repository @ commit | files counted | equations |
|---|---|---|---|
| cetta | CeTTa (zariuq/CeTTa) @ a9b3a811, lib/ and examples/ | 454 (1 duplicate file skipped) | 9,464 |
| petta | PeTTa (trueagi-io/PeTTa) @ ee5cacb6, lib/ | 21 | 319 |
| pettachainer | PeTTaChainer (rTreutlein/PeTTaChainer) @ 33cca343, every .metta file | 300 | 1,726 |
| wmtm | wmtm (bgoertzel-sing/wmtm) @ 9a8f693b, .metta files except tests/fixtures/ and reports/ (note-derived data and probe inputs) | 14 (5 duplicates skipped) | 5 |

Clones: projects/cettapack/scratch/survey/ (outside the repo; commits above). Command (paths shortened):

    python3 tools/survey_shapes.py --corpus cetta=cetta:lib,examples --corpus petta=<PeTTa>:lib --corpus pettachainer=<PeTTaChainer> --corpus wmtm=<wmtm> --exclude 'wmtm:tests/fixtures/*' --exclude 'wmtm:reports/*' --json evidence/fnspec-2026-11-18/survey/survey.json --md evidence/fnspec-2026-11-18/survey/survey.md --per-equation evidence/fnspec-2026-11-18/survey/equations.jsonl

Raw outputs: evidence/fnspec-2026-11-18/survey/survey.json (counts, examples), survey.md (all tables),
equations.jsonl (every equation: corpus, file:line, head, shape, its matches). No code text is copied into these
files or into this document; example locations are file:line in the public repositories at the commits above.

## Method

tools/survey_shapes.py reads each file as MeTTa s-expressions (';' comments, strings, '!' forms skipped) and takes
every top-level (= (head args...) body) as one equation. Evaluated heads are MeTTa builtins from a fixed list
(arithmetic, comparisons, if/case/let, collapse/superpose, list ops, effects, match) plus every head the same corpus
defines; any other expression head counts as data (a constructor). Two passes:

1. **Body shape** of every equation (first table): match-rooted bodies by template kind; bodies with a match below
   collapse, if/case or let; and the bodies without a match (recursion, conditionals, arithmetic, calls, data,
   effects).
2. **Every match occurrence** inside a function body (495): what its template evaluates (data; filter = (if C T (empty))
   with C comparisons on bound values and T data; arithmetic/comparison; if/case/let; calls to other functions;
   effects) and what the body does with its answers (stream them; collapse them into one tuple; collapse(once);
   an existence test (== (collapse ...) ()); fold/count). Aggregated into growth families (second table). A body is
   "whole-function" when it is that one match plus its wrapper and nothing else evaluates.

## Table 1: body shapes of all equations

| shape | cetta | petta | pettachainer | wmtm | total | example location (first occurrence per corpus) |
|---|---|---|---|---|---|---|
| match: single pattern + data template | 8 | 1 | 1 | 0 | 10 | cetta: lib/ilp/metagol_core.metta:75; petta: lib/lib_mm2.metta:13; pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:1213 |
| match: conjunctive/nested + data template | 1 | 0 | 0 | 0 | 1 | cetta: lib/ilp/metagol_core.metta:133 |
| match + filter (if on bound values, empty branch) | 0 | 0 | 0 | 0 | 0 |  |
| match + arithmetic/comparison on bound values | 0 | 0 | 0 | 0 | 0 |  |
| match + if/case/let on bound values (other) | 0 | 0 | 0 | 0 | 0 |  |
| match + template calling other functions | 10 | 1 | 0 | 0 | 11 | cetta: lib/petta/lib_spaces.metta:3; petta: lib/lib_spaces.metta:3 |
| match: template recursive (calls the function itself) | 0 | 0 | 0 | 0 | 0 |  |
| aggregate/collapse over a match | 5 | 2 | 39 | 0 | 46 | cetta: lib/lib_exec.metta:21; petta: lib/lib_spaces.metta:25; pettachainer: benchmarks/metta/benchgen_metta.metta:611 |
| if/case over a match | 10 | 2 | 84 | 0 | 96 | cetta: lib/embedding.metta:144; petta: lib/lib_he.metta:33; pettachainer: pettachainer/metta/compiler/rules.metta:31 |
| let/let* over a match | 26 | 1 | 124 | 0 | 151 | cetta: lib/embedding.metta:149; petta: lib/lib_datastructures.metta:40; pettachainer: pettachainer/metta/compiler/rules.metta:620 |
| other body containing a match | 11 | 0 | 47 | 0 | 58 | cetta: lib/lib_intensional_weighted.metta:20; pettachainer: benchmarks/metta/benchgen_metta.metta:573 |
| recursion over a list/structure | 854 | 21 | 117 | 2 | 994 | cetta: lib/dag.metta:23; petta: lib/lib_combinatorics.metta:22; pettachainer: benchmarks/metta/benchgen_metta.metta:30; wmtm: upstream/pettachainer/py1_nested_annotated_arrow.metta:9 |
| recursion (other) | 199 | 19 | 31 | 0 | 249 | cetta: lib/lang.metta:26; petta: lib/lib_combinatorics.metta:3; pettachainer: benchmarks/metta/benchgen_metta.metta:33 |
| if/case conditional (no match) | 1279 | 23 | 452 | 0 | 1754 | cetta: lib/dag.metta:16; petta: lib/lib_error.metta:4; pettachainer: benchmarks/metta/benchgen_metta.metta:39 |
| arithmetic/comparison expression | 249 | 23 | 47 | 1 | 320 | cetta: lib/embedding.metta:37; petta: lib/lib_datastructures.metta:22; pettachainer: benchmarks/metta/benchgen_metta.metta:42; wmtm: upstream/pettachainer/py1_nested_annotated_arrow.metta:11 |
| let/let* chain (no match) | 1307 | 32 | 279 | 0 | 1618 | cetta: lib/embedding.metta:51; petta: lib/lib_combinatorics.metta:10; pettachainer: benchmarks/metta/benchgen_metta.metta:111 |
| aggregate/collapse/superpose (no match) | 73 | 7 | 30 | 0 | 110 | cetta: lib/lib_evidence_ledger.metta:244; petta: lib/lib_combinatorics.metta:15; pettachainer: pettachainer/metta/compiler/query_lowering.metta:35 |
| call to other functions | 3626 | 122 | 214 | 0 | 3962 | cetta: lib/embedding.metta:39; petta: lib/lib_datastructures.metta:6; pettachainer: benchmarks/metta/benchgen_metta.metta:93 |
| data expression (no evaluation) | 1037 | 23 | 88 | 1 | 1149 | cetta: lib/abt_default_signatures.metta:6; petta: lib/lib_combinatorics.metta:33; pettachainer: benchmarks/metta/benchgen_metta.metta:23; wmtm: upstream/pettachainer/py2_variable_effect_arrow.metta:7 |
| constant (symbol/variable/value) | 702 | 31 | 81 | 1 | 815 | cetta: lib/gparse.metta:156; petta: lib/lib_he.metta:87; pettachainer: benchmarks/metta/benchgen_metta.metta:29; wmtm: upstream/pettachainer/py1_nested_annotated_arrow.metta:8 |
| side effects (no match) | 67 | 11 | 92 | 0 | 170 | cetta: lib/embedding.metta:143; petta: lib/lib_he.metta:25; pettachainer: benchmarks/metta/benchgen_metta.metta:108 |
| **all equations** | 9464 | 319 | 1726 | 5 | 11514 | |
| files (distinct) | 454 | 21 | 300 | 14 | | |

Only 22 of 11,514 equations have a match at the root of the body; the probe's own shape (a match whose template is
a filter) occurs 0 times as a whole body. Matches in real code sit under collapse, let, case and existence tests:
pass 2 counts them where they are.

## Table 2: growth families of the 495 match occurrences

(pass 2 aggregated): what the body does with the answers x what the template does.
compilable: the template evaluates only comparisons, arithmetic, if/case/let and empty on bound values;
calls (other user functions) and effects are outside the fragment.

| rank | answers | template | compilable | cetta | petta | pettachainer | wmtm | total occurrences | of them whole-function bodies |
|---|---|---|---|---|---|---|---|---|---|
| 1 | collapse (all answers as one tuple) | data | yes | 12 | 0 | 72 | 0 | 84 | 10 |
|  | collapse (all answers as one tuple) | calls | no | 1 | 0 | 81 | 0 | 82 | 12 |
| 2 | stream (answers returned one by one) | data | yes | 34 | 3 | 23 | 0 | 60 | 15 |
|  | stream (answers returned one by one) | calls | no | 15 | 0 | 22 | 0 | 37 | 8 |
|  | collapse (all answers as one tuple) | effects | no | 3 | 1 | 32 | 0 | 36 | 9 |
| 3 | collapse (all answers as one tuple) | filter | yes | 0 | 0 | 34 | 0 | 34 | 5 |
| 4 | existence test | filter | yes | 0 | 0 | 28 | 0 | 28 | 5 |
| 5 | collapse(once) (first answer) | data | yes | 0 | 0 | 28 | 0 | 28 | 1 |
|  | collapse(once) (first answer) | calls | no | 0 | 0 | 25 | 0 | 25 | 10 |
| 6 | existence test | data | yes | 7 | 1 | 15 | 0 | 23 | 10 |
| 7 | collapse(once) (first answer) | filter | yes | 0 | 0 | 19 | 0 | 19 | 0 |
|  | existence test | calls | no | 0 | 0 | 11 | 0 | 11 | 2 |
| 8 | stream (answers returned one by one) | filter | yes | 1 | 0 | 6 | 0 | 7 | 1 |
|  | stream (answers returned one by one) | effects | no | 5 | 1 | 0 | 0 | 6 | 3 |
| 9 | collapse(once) (first answer) | if/case/let | yes | 0 | 0 | 4 | 0 | 4 | 0 |
| 10 | fold/count | data | yes | 2 | 1 | 0 | 0 | 3 | 3 |
| 11 | existence test | if/case/let | yes | 0 | 0 | 3 | 0 | 3 | 1 |
| 12 | stream (answers returned one by one) | if/case/let | yes | 1 | 0 | 2 | 0 | 3 | 0 |
|  | collapse(once) (first answer) | effects | no | 0 | 0 | 2 | 0 | 2 | 1 |

(Rank counts only the families the probe's fragment can grow into: a template that evaluates nothing but
comparisons, arithmetic, if/case/let and empty on bound values. Calls to other user functions need those functions
compiled too; effects are outside a read-only fragment.)

## The top three shapes, and why

Ranked by occurrences among the compilable families:

1. **collapse + data template** (84 occurrences; 10 whole-function bodies): (collapse (match S P T)), T data. One
   answer, the tuple of every row's template, in CeTTa's row order.
2. **stream + data template** (60; 15 whole bodies): (match S P T), T data. One answer per row. This is the bare
   match of M1/the probes, now as a whole function.
3. **collapse + filter** (34; 5 whole bodies): (collapse (match S P (if C T (empty)))), C a comparison or and/or/not of
   comparisons (==, =alpha, <, >, <=, >=) on bound values and arguments. The probe's filter, under collapse, as
   PeTTaChainer writes it.

Why these and not the next ones: existence test + filter (28) and collapse(once) + data (28) come next. Existence tests
are a collapse(once) compared with (), and collapse(once) returns the first row, so both depend on which row CeTTa
produces first; the three chosen shapes depend only on the bag (stream) or on the full row order, which the bag tests
compare directly. Arithmetic on bound values occurs 0 times as a template in these corpora (Table 2 has no
arithmetic family), so no chosen shape is arithmetic; the compiler still handles numeric comparisons (<, >, <=, >=,
== on numbers) in a filter condition, and the bag tests cover their edge values. Recursion over lists (994 equations)
and calls to other functions (3,962) are the largest body classes overall but contain no match: the probe's fragment
cannot grow into them.

The probe's own shape (stream + filter, 7 occurrences, rank 8) stays compiled for the four probe sites so that
their earlier answers can be checked unchanged.

## Real-code sites, one per chosen shape (benchmark step 5)

| shape | site | location |
|---|---|---|
| collapse + data | live-hypotheses: the hypotheses of a KB from the &hm_live rows | PeTTaChainer tests/metta/test_hypothesis_mining.metta:7 |
| stream + data | mil:run on a fact primitive: two nested matches on the same space, the fact's output | CeTTa lib/ilp/metagol_core.metta:133 |
| collapse + filter | base-rate-cache-mode-rows: cache rows for (hash, kb) whose statement is alpha-equal to the pattern and whose mode is any or the asked one | PeTTaChainer pettachainer/metta/runtime/statistics/base_rates.metta:41 |

Each site takes the one equation (in CeTTa's he language, without the PeTTa type declaration) and runs on a generated
space with the pattern's row shape (bench/gen_fnspec_kb.py); in its home program the function may have sibling
equations (mil:run has three), in which case the definition guard sends calls to the residual, as it should.

## Limits

- Static and syntactic: a head is "evaluated" when it is a listed builtin or defined in the same corpus; heads defined
  in an imported library of another corpus count as data. A variable-headed template ($f $x) counts as data (CeTTa
  evaluates it when $f is bound to a function; the compiled code checks that per row).
- "Whole-function" requires every other argument of the wrapper chain to evaluate nothing; let/case bodies whose other
  parts call functions defined outside the corpus can pass as whole bodies (a few false positives, e.g. a let whose
  body calls an imported map function).
- Counts are of source equations, not of calls at run time; hotness is not measured.
- wmtm has almost no MeTTa code at this commit (5 equations); its counts do not change the ranking.
