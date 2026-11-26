| test | off = on | off = invalid | routes | verdict |
|---|---|---|---|---|
| t1_values | yes | yes | offered 47 answered 35 declined: invalid 0, call 12; invalid mode forced 124/124 | pass |
| t2_proof_store | yes | yes | offered 31 answered 25 declined: invalid 0, call 6; invalid mode forced 47/47 | pass |
| t3_redefine_self | yes | yes | offered 22 answered 5 declined: invalid 17, call 0; invalid mode forced 36/36 MISMATCH: tv-confidence answered 2 < 4; make-and-bundle answered 0 < 2; frontier-pool-potential answered 0 < 1; make-and-bundle declined_invalid 0 < 2; no decline reason containing 'constructor'; no decline reason containing 'differs from the compiled text' | FAIL |
| t4_redefine_module | yes | yes | offered 14 answered 4 declined: invalid 10, call 0; invalid mode forced 20/20 MISMATCH: proof-output-key-by-id answered 3 < 6; tv-confidence answered 0 < 1; tv-confidence declined_invalid 0 < 1 | FAIL |
| t5_proof_data | yes | yes | offered 7 answered 3 declined: invalid 0, call 4; invalid mode forced 11/11 | pass |
| t6_types | yes | yes | offered 12 answered 7 declined: invalid 0, call 5; invalid mode forced 15/15 | pass |
