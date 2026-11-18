# Whole-function specialisation on CeTTa's own matcher: plan

Task t-2026-11-18-cettapack-fnspec (andy, Work msg 773). Branch hive/t-2026-11-18-cettapack-fnspec, worktree
wt/fnspec, from hive/t-2026-11-18-cettapack-next-step-probe cff658d (cettapack main is 64fa80d and does not hold the
probe branch yet). CeTTa a9b3a811 + upstream/cetta 0001-0003 as files, unchanged. Ben's note:
docs/cetta-guarded-supercompilation-2026-07-06.pdf (sha256 757a2539...0ebd0cb2; text copy used for section numbers:
scratch/ben-notes/cetta-guarded-supercompilation-2026-07-06.txt, outside the repo). No PathMap index in this milestone.

This file was committed before the shape survey and the compiler (acceptance 1). Section numbers below are the
note's.

## What the milestone builds, mapped to the note

| # | part of this milestone | note section | how it is built here |
|---|---|---|---|
| B1 | **Function-level specialisation**: a MeTTa function whose body is one match plus a wrapper (stream or collapse) and a template of data, comparisons on bound values, if/empty, becomes one compiled C callable | 1 (hot, stable, semantically classifiable regions replaced by guarded artifacts); 3 (pipeline: normalise -> choose presentation -> specialise -> pack); 7 (presentation "native C: exact control, direct result construction, arena-local allocation") | tools/fnspecc.py reads the definition, classifies the body into one of the three shapes chosen by the survey (docs/FNSPEC_SURVEY.md), refuses anything else, and emits C: the row loop, the filter as C code over bound values, the template by bindings_apply, the wrapper (stream / collapse) and CeTTa's result conventions |
| B2 | **CeTTa's matcher, no index** | 4.3 (index chooses candidates, CeTTa's native matcher decides exact bindings, kernel builds answers); 6.4 (exact rematch is the runtime seam) | the generated code calls space_query_conjunction on the live space and bindings_apply for answers; no PathMap or MORK narrowing (the probes found CeTTa's matcher as fast without the 520-775 MiB index). The note's step 2/3 of pattern 1 (10) is therefore not built; its step 5-8 are |
| B3 | **Code-shape guard per revision of &self** | 8.2 (dependency vector: normalized/meta-expanded region hash), 8.3 (meta-rule invalidation, fail-closed), 14 (structural hashes), A.3 (guard pseudocode: current_meta_expanded_hash(...) != pack hash -> INVALID), A.5 (structural hash after normalising variable names) | on every call the guard reads &self's (instance_id, equation_revision) (CeTTa src/space.h); when either moved since the last check, it re-reads every equation of the function from &self and compares it, variables renamed by first occurrence, with the compiled one (exactly one equation, same structure, same atoms); a mismatch leaves the guard INVALID until the next revision. Not checked once at load |
| B4 | **Builtin-meaning guard** | 8.2 (operator_schema_hashes, profile_digest), 8.4 (profile mismatch is INVALID), A.10 ("do not assume profile upgrades preserve kernel validity") | per &self revision: no equation in &self can rewrite a call the compiled body relies on (if, ==, =alpha, and, or, not, <, >, <=, >=, collapse, match, empty): the guard queries &self for (= (<op> $x ...) $r) at each used arity, which also catches a catch-all (= $x ...); any hit -> INVALID. The engine profile is the embedding's (he), fixed at engine creation |
| B5 | **Value-kind guards per row** | 8.4 (guard states), 10.2 (correctness contract: template instantiation matches generic CeTTa), A.1 ("unsupported cases should refuse compilation or deopt, not guess") | arguments: ground and in normal form; bound values a filter compares: symbols or numbers as the comparison needs (int with int, float with float); any other kind aborts the call to the residual, before any answer is handed back |
| B6 | **Rows in normal form** | 10.2 (result template instantiation matches generic CeTTa) | CeTTa evaluates each instantiated template; the compiled code does not. Per &self revision the guard builds the set of symbols that head an equation in &self; per row, every instantiated answer is walked: no variable, no grounded operation, no symbol in that set (else the call aborts to the residual) |
| B7 | **Residual** | 3 (invalid -> fallback), 6.5 (deoptimisation is a host responsibility), A.4 (patch lifecycle: INVALID -> run_fallback), A.10 ("do not remove the residual fallback") | the residual is CeTTa evaluating the MeTTa function itself, (f args...), in the same engine; taken on any guard failure, any per-row abort, or a forced-INVALID guard (test mode) |
| B8 | **CeTTa's result conventions** | 10.1 (multiplicity protocol: duplicates are real, no dedup), 10.2 | bags with multiplicity, as the matcher returns rows; stream functions whose every row takes the (empty) branch return one Empty per row (the probe's rule, docs/NEXT_STEP_PROBES.md); collapse returns one tuple in CeTTa's row order (checked by the bag tests, not assumed) |
| B9 | **Bag tests** | 16.1 (phase 1: differential testing), A.9 (duplicates, co-reference, distinct variables, empty bags, structural-hash mismatch fallback, comparison with generic CeTTa) | every shape and site against CeTTa evaluating the same function in its own engine: the probes' 6 small spaces, numeric edge spaces, recursion through a MeTTa caller, a forced-INVALID guard, a redefinition of the function and of a builtin in &self mid-run |
| B10 | **Benchmark with guard overhead included and the residual baseline** | 17 (baselines: generic CeTTa, compiled with guards, residual fallback; "never report only the generated kernel time") | the probes' method (pop-os, M1 KB, RUNS=7 REPEAT=5, interleaved, prebuilt atoms, paired ratios, peak RSS, route) with modes fn-generic, fn-hosted (the function with its match hosted on CeTTa's matcher), fn-spec (guards included), fn-fallback |

## What it leaves, and why

| part of the note | section | left because |
|---|---|---|
| PathMap/MORK candidate narrowing, product zipper / trie join, index artifacts | 4.3, 10 steps 2-4, 8.1 pathmap_artifacts | stop-line of the order; the probes found CeTTa's matcher as fast without the index |
| PATCHABLE and REPLAN guard states, append-only patching, statistics drift, amortisation thresholds | 8.4, 15, A.3-A.4 | nothing here is patchable: a changed definition is INVALID (residual) until it is changed back; no index to patch |
| Space schema hashes, cross-space guards, space revision vectors for data spaces | 8.2 spaces, 9 | the compiled code reads the live data space through CeTTa's matcher on every call, so data revisions need no guard; only &self (code) is guarded |
| MG-IR planner, cost model, profiler of hot regions | 3, 7, 20 milestone 0 | the three shapes are chosen from the static survey, not from runtime profiles |
| Effect algebra; bodies with add-atom/remove-atom or other effects | 9, 4.4 | the compiled fragment is read-only; bodies with effects are refused by the compiler (they stay CeTTa's) |
| Search episodes, PeTTa delegation, PLN factor kernels, MM2 lowering, tensors | 11, 12, 13, 19 | other milestones of the note's roadmap (20: 5-8) |
| Certificates beyond a manifest, Lean checking | 16.2-16.3, A.8 | the claim level here is bag-tested-adequate (A.8 vocabulary); each compiled function records its body digest and guard list in a generated manifest comment |
| A CeTTa entry point for specialised code | 6.5, 4.1 | stop-line: the entry point stays an embedding callback (0003 offered to Zar separately) |
| Shapes outside the top three (recursion over lists, bodies calling other functions, collapse(once), existence tests, fold/count) | - | the order asks for three; the survey's ranking and the reasons are in docs/FNSPEC_SURVEY.md |

## Steps and order of commits

1. this plan (committed first);
2. the shape survey: tools/survey_shapes.py, evidence/fnspec-2026-11-18/survey/, docs/FNSPEC_SURVEY.md with the
   top three shapes and why (committed before any compiler code);
3. the compiler and runtime: tools/fnspecc.py (definition -> C), src/cpk_fnspec.c (guards, residual, callable),
   src/cpk_fnspec_host.c (bench, difftest), fnspec/sites/*.metta (the probe sites in each shape, one real-code site per
   shape from the survey);
4. bag tests per shape and site (make fnspec-difftest; part of make test);
5. benchmark (bench/run_fnspec.sh, bench/summarize_fnspec.py; tables in docs/FNSPEC.md; raw JSONL in evidence/).

## Acceptance as read here

- shapes follow from the survey's counts (ranked table, top three among the shapes the fragment can grow into);
- 0 bag-test failures on every shape and site; the forced-INVALID guard and a mid-run redefinition each take the
  residual and give CeTTa's answers;
- per shape and site, a table with the probe's columns, reproduced by recomputing from the raw runs;
- make test passes; the four probe sites' earlier answers are unchanged (the probe difftest still passes).
