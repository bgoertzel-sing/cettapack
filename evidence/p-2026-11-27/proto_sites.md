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

