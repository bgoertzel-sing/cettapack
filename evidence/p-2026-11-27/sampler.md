| phase | w1-examples timers / sampler | w2-wmtm timers / sampler | w3-benchgen timers / sampler |
|---|---|---|---|
| dispatch | 27.6% / 26.9% | 29.0% / 28.5% | 28.5% / 27.6% |
| choice | 2.4% / 2.3% | 2.6% / 2.4% | 2.5% / 3.4% |
| eqsel | 24.2% / 23.8% | 20.7% / 20.7% | 17.3% / 17.2% |
| prepcompile | 4.1% / 3.9% | 3.2% / 3.2% | 6.2% / 5.9% |
| prepexec | 2.0% / 1.9% | 1.6% / 1.6% | 1.6% / 1.5% |
| match | 9.2% / 8.7% | 11.2% / 10.5% | 13.8% / 12.4% |
| copy | 10.6% / 10.3% | 10.8% / 10.5% | 11.1% / 10.9% |
| alloc | 8.9% / 11.2% | 8.9% / 11.1% | 9.1% / 11.5% |
| space | 4.7% / 4.6% | 5.7% / 5.5% | 4.9% / 4.7% |
| prolog | 4.9% / 5.2% | 5.9% / 5.9% | 4.3% / 4.4% |
| outside | 1.1% / 1.2% | 0.3% / 0.3% | 0.7% / 0.6% |
| samples (3 runs) | 188130 | 105066 | 315564 |

S1's native categories (innermost named function, bench/s1/sample_categories.py), same samples:

| category | w1-examples | w2-wmtm | w3-benchgen |
|---|---|---|---|
| matching, unification and fact iteration | 22.7% | 24.1% | 24.5% |
| equation selection and call preparation | 20.7% | 17.0% | 17.6% |
| goal dispatch and evaluation | 15.5% | 15.9% | 15.7% |
| allocation and freeing | 13.8% | 13.9% | 14.2% |
| copying and instantiation | 9.8% | 10.3% | 10.3% |
| hashing, interning and term universe | 9.0% | 9.8% | 9.4% |
| prolog (SWI-Prolog and the foreign boundary) | 5.1% | 5.7% | 4.4% |
| grounded ops and arithmetic | 1.4% | 1.4% | 1.7% |
| choice points, trail and backtracking | 1.3% | 1.1% | 1.4% |
| space mutation (add/remove atoms) | 0.7% | 0.9% | 0.7% |
| other machine | 0.0% | 0.0% | 0.1% |
| profiler | 0.0% | 0.0% | 0.0% |
