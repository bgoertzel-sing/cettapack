# Native primitives for PeTTaChainer's frontier heaps and proof store, measured where the time goes

Task t-2026-11-26-cettapack-native-primitives (goal 2026-11-24-cettapack-roadmap; andy; M2, the loader, on hold).
Branch hive/t-2026-11-26-cettapack-native-primitives. **Started from the S1 branch head f5bac2a (main efcfdd3 + S0
f8126a1 + S1), not main**: neither S0 nor S1 is merged into main.

<!-- SUMMARY -->

## 1. Inputs and pins

| what | pin | note |
|---|---|---|
| cettapack | f5bac2a (S1 head = main efcfdd3 + S0 f8126a1 + S1) | this branch |
| CeTTa | a9b3a811 + upstream/cetta/0001-0004 + upstream/cetta-s0/0005 (files, unchanged) | as S0 |
| this task's patches (files) | upstream/cetta-np/0009 (profiler: Prolog-call timing), 0010 (native heaps) | apply with `git am` on 0001-0005; with S1's 0006/0007 also, with `git am --3way` (no conflicts) |
| SWI-Prolog | 10.0.2, user-local (scratch/deps/swipl10) | as S0 |
| PeTTaChainer | b0e24f9b (pettachainer/metta subtree), files unchanged | as S0 |
| PeTTa | e038e4db, lib/lib_*.metta overlaid | as S0 |
| workloads | S0's w1-w3, unchanged programs | bench/s0/gen_workloads.py, seed 20261124 |

Builds (bench/np/build_np_cetta.sh, trees under build/np; sha256 in evidence/np-2026-11-26/binaries.txt):
`prof` = 0001-0006 + 0009 + 0010, CETTA_FNPROF=1, -g (profiles, Prolog-call timing, samples); `on` = 0001-0005 + 0009 +
0010, CETTA_FNPROF=1 (S0's profiled configuration, used for both arms of the benchmark); `off` = the same with
CETTA_FNPROF=0 (profiler compiled out, for the profiler-off pair).

<!-- REST -->
