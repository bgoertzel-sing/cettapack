| test | off = on | off = invalid | routes (on, then invalid mode) | verdict |
|---|---|---|---|---|
| t1_basic | yes | yes | recognised 11, specialised 11, residual 0  | forced 11/11 | MISMATCH: recognised=11 (want 12); specialised=11 (want 12) | FAIL |
| t2_values | yes | yes | recognised 6, specialised 5, residual 1 {"residual.inputs": 1} | forced 6/6 | MISMATCH: recognised=6 (want 7) | FAIL |
| t3_order | yes | yes | recognised 2, specialised 1, residual 1 {"residual.order": 1} | forced 2/2 | MISMATCH: recognised=2 (want 3) | FAIL |
| t4_redefine | yes | yes | recognised 9, specialised 5, residual 4 {"residual.invalid": 4} | forced 9/9 | MISMATCH: recognised=9 (want 10); specialised=5 (want 6) | FAIL |
| t5_replace | yes | yes | recognised 3, specialised 1, residual 2 {"residual.invalid": 2} | forced 3/3 | MISMATCH: recognised=3 (want 4); specialised=1 (want 2) | FAIL |
