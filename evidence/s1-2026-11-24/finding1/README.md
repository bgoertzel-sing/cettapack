# S1: S0 review finding 1 (w2 goal check)

- s0-runs-goals.md: bench/s1/check_w2_goals.py on S0's nine w2 runs (evidence/s0-2026-11-24/runs/{base,off,on}/w2-wmtm),
  program s0_w2_wmtm.steps100.metta (= S0's, sha256 ec4a904c...).
- steps100/: bench/s1/run_workloads.sh base on S1's base build (a9b3a811 + 0001-0004, ENABLE_LIB_PROLOG=1); stdout
  byte-identical to S0's (check file); goals.md.
- steps1000/: the same program with --steps 1000 (W2_ARGS="--steps 1000"; s0_w2_wmtm.steps1000.metta, sha256 79e9ead2...);
  goals.md. A 3,000-step run was started and stopped after a minute (not used).

Commands:
    bench/s1/run_workloads.sh base evidence/s1-2026-11-24/finding1/steps100 w2-wmtm
    W2_ARGS="--steps 1000" bench/s1/run_workloads.sh base evidence/s1-2026-11-24/finding1/steps1000 w2-wmtm
    python3 bench/s1/check_w2_goals.py <program> <stdout>... > goals.md
