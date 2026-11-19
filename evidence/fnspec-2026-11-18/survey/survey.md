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

Match occurrences inside function bodies (pass 2):

| context of the match | template | cetta | petta | pettachainer | wmtm | total | example location |
|---|---|---|---|---|---|---|---|
| collapse inside other code | calls | 1 | 0 | 61 | 0 | 62 | cetta: lib/lib_exec.metta:26; pettachainer: pettachainer/metta/runtime/hypothesis_review.metta:104 |
| collapse inside other code | data | 2 | 0 | 56 | 0 | 58 | cetta: lib/petta/lib_transform.metta:329; pettachainer: benchmarks/metta/benchgen_metta.metta:573 |
| collapse inside other code | filter | 0 | 0 | 26 | 0 | 26 | pettachainer: pettachainer/metta/runtime/hypothesis_review.metta:136 |
| existence test (collapse(once) == ()) | filter | 0 | 0 | 25 | 0 | 25 | pettachainer: pettachainer/metta/compiler/query_lowering.metta:153 |
| collapse inside other code | effects | 0 | 0 | 23 | 0 | 23 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:183 |
| other | data | 12 | 0 | 7 | 0 | 19 | cetta: lib/lib_intensional_weighted.metta:28; pettachainer: pettachainer/metta/compiler/rules.metta:644 |
| bound by let | data | 9 | 0 | 9 | 0 | 18 | cetta: lib/ilp/metagol_prime.metta:136; pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:183 |
| collapse(once) inside other code | data | 0 | 0 | 15 | 0 | 15 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:67 |
| collapse bound by let | data | 6 | 0 | 8 | 0 | 14 | cetta: lib/ilp/metagol_core.metta:255; pettachainer: pettachainer/metta/compiler/source_lowering.metta:2 |
| existence test (collapse(once) == ()) | data | 3 | 1 | 9 | 0 | 13 | cetta: lib/petta/lib_datastructures.metta:22; petta: lib/lib_datastructures.metta:40; pettachainer: pettachainer/metta/compiler/rules.metta:31 |
| bound by let | calls | 3 | 0 | 10 | 0 | 13 | cetta: lib/lib_exec.metta:52; pettachainer: pettachainer/metta/compiler/rules.metta:688 |
| body | data | 9 | 1 | 1 | 0 | 11 | cetta: lib/ilp/metagol_core.metta:75; petta: lib/lib_mm2.metta:13; pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:1213 |
| existence test (collapse(once) == ()) | calls | 0 | 0 | 11 | 0 | 11 | pettachainer: pettachainer/metta/runtime/installation.metta:129 |
| collapse(once) as case scrutinee | data | 0 | 0 | 11 | 0 | 11 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:215 |
| other | calls | 4 | 0 | 6 | 0 | 10 | cetta: lib/stdlib.metta:728; pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:814 |
| existence test (collapse == ()) | data | 4 | 0 | 6 | 0 | 10 | cetta: lib/petta/lib_ic_mil_export.metta:40; pettachainer: pettachainer/metta/compiler/rules.metta:604 |
| collapse bound by let | calls | 0 | 0 | 10 | 0 | 10 | pettachainer: pettachainer/metta/runtime/installation.metta:150 |
| collapse as the body | calls | 0 | 0 | 10 | 0 | 10 | pettachainer: benchmarks/metta/benchgen_metta.metta:629 |
| collapse(once) as the body | calls | 0 | 0 | 10 | 0 | 10 | pettachainer: pettachainer/metta/runtime/backward/eval_cache.metta:6 |
| collapse(once) as case scrutinee | filter | 0 | 0 | 9 | 0 | 9 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:1071 |
| collapse(once) inside other code | filter | 0 | 0 | 8 | 0 | 8 | pettachainer: pettachainer/metta/compiler/rules.metta:620 |
| collapse bound by let | effects | 0 | 0 | 8 | 0 | 8 | pettachainer: pettachainer/metta/runtime/kb_publication.metta:110 |
| body | calls | 8 | 0 | 0 | 0 | 8 | cetta: examples/counterexample_loom.metta:34 |
| collapse(once) inside other code | calls | 0 | 0 | 8 | 0 | 8 | pettachainer: pettachainer/metta/runtime/installation.metta:83 |
| collapse(once) as case scrutinee | calls | 0 | 0 | 7 | 0 | 7 | pettachainer: pettachainer/metta/runtime/evaluator/core.metta:239 |
| collapse as case scrutinee | data | 4 | 0 | 3 | 0 | 7 | cetta: lib/embedding.metta:144; pettachainer: pettachainer/metta/runtime/evaluator/sampling.metta:70 |
| case scrutinee | data | 3 | 2 | 0 | 0 | 5 | cetta: lib/petta/lib_he.metta:25; petta: lib/lib_he.metta:33 |
| collapse as the body | data | 0 | 0 | 5 | 0 | 5 | pettachainer: benchmarks/metta/benchgen_metta.metta:611 |
| collapse as the body | effects | 3 | 1 | 1 | 0 | 5 | cetta: lib/lib_exec.metta:21; petta: lib/lib_spaces.metta:25; pettachainer: pettachainer/metta/runtime/installation.metta:172 |
| under if/case | calls | 0 | 0 | 5 | 0 | 5 | pettachainer: pettachainer/metta/runtime/application_discovery/plans.metta:22 |
| collapse bound by let | filter | 0 | 0 | 5 | 0 | 5 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:103 |
| other | filter | 1 | 0 | 3 | 0 | 4 | cetta: examples/chaining/tutorial/backward-core.metta:5; pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:788 |
| under if/case | data | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/runtime/backward/upward.metta:93 |
| collapse(once) as case scrutinee | if/case/let | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:83 |
| collapse as the body | filter | 0 | 0 | 3 | 0 | 3 | pettachainer: pettachainer/metta/runtime/statistics/base_rates.metta:41 |
| once | data | 1 | 0 | 2 | 0 | 3 | cetta: lib/lib_exec.metta:33; pettachainer: pettachainer/metta/runtime/evaluator/values.metta:828 |
| fold/count | data | 2 | 1 | 0 | 0 | 3 | cetta: lib/petta/lib_spaces.metta:28; petta: lib/lib_spaces.metta:30 |
| existence test (collapse(once) == ()) | if/case/let | 0 | 0 | 3 | 0 | 3 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:643 |
| body | effects | 2 | 1 | 0 | 0 | 3 | cetta: lib/petta/lib_spaces.metta:3; petta: lib/lib_spaces.metta:3 |
| other | effects | 3 | 0 | 0 | 0 | 3 | cetta: lib/lib_intensional_weighted.metta:20 |
| existence test (collapse == ()) | filter | 0 | 0 | 3 | 0 | 3 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:569 |
| collapse(once) bound by let | filter | 0 | 0 | 2 | 0 | 2 | pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:239 |
| bound by let | if/case/let | 0 | 0 | 2 | 0 | 2 | pettachainer: pettachainer/metta/runtime/evaluator/values.metta:828 |
| under if/case | if/case/let | 1 | 0 | 0 | 0 | 1 | cetta: examples/atp/prime/direct_search.metta:52 |
| once | filter | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/statistics/ledger.metta:49 |
| bound by let | filter | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:170 |
| under if/case | filter | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/statistics/aggregates.metta:121 |
| collapse(once) as the body | data | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/proofs.metta:150 |
| collapse(once) bound by let | data | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/store/value_store.metta:201 |
| collapse(once) as the body | effects | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:17 |
| collapse(once) bound by let | effects | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:55 |
| once | calls | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/evaluator/values.metta:798 |
| **all** | | 81 | 7 | 407 | 0 | 495 | |

Conjunctive or nested matches among them: cetta 7, petta 0, pettachainer 12, wmtm 0

Growth families (pass 2 aggregated): what the body does with the answers x what the template does.
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

Whole-function bodies: one match plus a wrapper that evaluates nothing else (pass 2):

| context of the match | template | cetta | petta | pettachainer | wmtm | total | example location |
|---|---|---|---|---|---|---|---|
| body | data | 9 | 1 | 1 | 0 | 11 | cetta: lib/ilp/metagol_core.metta:75; petta: lib/lib_mm2.metta:13; pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:1213 |
| collapse(once) as the body | calls | 0 | 0 | 10 | 0 | 10 | pettachainer: pettachainer/metta/runtime/backward/eval_cache.metta:6 |
| body | calls | 8 | 0 | 0 | 0 | 8 | cetta: examples/counterexample_loom.metta:34 |
| collapse as the body | calls | 0 | 0 | 8 | 0 | 8 | pettachainer: benchmarks/metta/benchgen_metta.metta:629 |
| existence test (collapse(once) == ()) | data | 0 | 0 | 6 | 0 | 6 | pettachainer: pettachainer/metta/lib/collections.metta:13 |
| collapse as the body | data | 0 | 0 | 5 | 0 | 5 | pettachainer: benchmarks/metta/benchgen_metta.metta:611 |
| collapse as the body | effects | 3 | 1 | 1 | 0 | 5 | cetta: lib/lib_exec.metta:21; petta: lib/lib_spaces.metta:25; pettachainer: pettachainer/metta/runtime/installation.metta:172 |
| collapse bound by let | effects | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/runtime/kb_publication.metta:110 |
| existence test (collapse(once) == ()) | filter | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/compiler/query_lowering.metta:153 |
| existence test (collapse == ()) | data | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/core/logic_config.metta:49 |
| collapse bound by let | calls | 0 | 0 | 4 | 0 | 4 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:122 |
| collapse as the body | filter | 0 | 0 | 3 | 0 | 3 | pettachainer: pettachainer/metta/runtime/statistics/base_rates.metta:41 |
| case scrutinee | data | 2 | 1 | 0 | 0 | 3 | cetta: lib/petta/lib_spaces.metta:9; petta: lib/lib_spaces.metta:9 |
| fold/count | data | 2 | 1 | 0 | 0 | 3 | cetta: lib/petta/lib_spaces.metta:28; petta: lib/lib_spaces.metta:30 |
| body | effects | 2 | 1 | 0 | 0 | 3 | cetta: lib/petta/lib_spaces.metta:3; petta: lib/lib_spaces.metta:3 |
| collapse bound by let | data | 0 | 0 | 2 | 0 | 2 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:1155 |
| existence test (collapse(once) == ()) | calls | 0 | 0 | 2 | 0 | 2 | pettachainer: pettachainer/metta/runtime/evaluator/versions.metta:367 |
| collapse as case scrutinee | data | 2 | 0 | 0 | 0 | 2 | cetta: lib/embedding.metta:144 |
| collapse bound by let | filter | 0 | 0 | 2 | 0 | 2 | pettachainer: pettachainer/metta/runtime/store/journal.metta:10 |
| once | filter | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/statistics/ledger.metta:49 |
| once | data | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/statistics/ledger.metta:77 |
| collapse(once) as the body | data | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/proofs.metta:150 |
| collapse inside other code | data | 0 | 0 | 1 | 0 | 1 | pettachainer: tests/metta/test_answers_by_index.metta:8 |
| existence test (collapse(once) == ()) | if/case/let | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/backward/skeleton.metta:643 |
| collapse(once) as the body | effects | 0 | 0 | 1 | 0 | 1 | pettachainer: pettachainer/metta/runtime/hypothesis_mining.metta:17 |
| existence test (collapse == ()) | filter | 0 | 0 | 1 | 0 | 1 | pettachainer: tests/metta/test_hypothesis_reconsider.metta:34 |
| **all** | | 28 | 5 | 63 | 0 | 96 | |
