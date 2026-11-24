| test | off = on | off = invalid | routes (on, then invalid mode) | verdict |
|---|---|---|---|---|
| t0_first_call | yes | yes | recognised 3, specialised 3, residual 0  | forced 3/3 | pass |
| t1_basic | yes | yes | recognised 11, specialised 11, residual 0  | forced 11/11 | pass |
| t2_values | yes | yes | recognised 6, specialised 5, residual 1 {"residual.inputs": 1} | forced 6/6 | pass |
| t3_order | yes | yes | recognised 2, specialised 1, residual 1 {"residual.order": 1} | forced 2/2 | pass |
| t4_redefine | yes | yes | recognised 9, specialised 5, residual 4 {"residual.invalid": 4} | forced 9/9 | pass |
| t5_replace | yes | yes | recognised 3, specialised 1, residual 2 {"residual.invalid": 2} | forced 3/3 | pass |
| t6_value | yes | yes | recognised 3, specialised 2, residual 1 {"residual.value": 1} | forced 3/3 | pass |
| t7_carriers | yes | yes | recognised 5, specialised 5, residual 0  | forced 5/5 | pass |
