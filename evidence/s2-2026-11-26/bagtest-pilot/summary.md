| test | off = on | off = invalid | routes | verdict |
|---|---|---|---|---|
| t1_values | yes | yes | offered 47 answered 35 declined: invalid 0, call 12; invalid mode forced 124/124 MISMATCH: tv-confidence answered 7 < 8; proof-score answered 1 < 2; expr-arity answered 0 < 3 | FAIL |
| t2_proof_store | yes | yes | offered 31 answered 25 declined: invalid 0, call 6; invalid mode forced 47/47 MISMATCH: proof-output-key-by-id declined_call 2 < 3 | FAIL |
| t3_redefine_self | yes | yes | offered 17 answered 3 declined: invalid 14, call 0; invalid mode forced 31/31 MISMATCH: tv-confidence answered 0 < 4; make-and-bundle answered 0 < 1; expr-arity answered 0 < 1; make-and-bundle declined_invalid 0 < 1; expr-arity declined_invalid 0 < 1; no decline reason containing 'constructor'; no decline reason containing 'type declaration' | FAIL |
| t4_redefine_module | yes | yes | offered 9 answered 3 declined: invalid 6, call 0; invalid mode forced 15/15 MISMATCH: tv-confidence answered 0 < 2; tv-confidence declined_invalid 0 < 1 | FAIL |
| t5_proof_data | yes | yes | offered 7 answered 3 declined: invalid 0, call 4; invalid mode forced 11/11 | pass |
| t6_types | yes | yes | offered 12 answered 7 declined: invalid 0, call 5; invalid mode forced 15/15 | pass |
