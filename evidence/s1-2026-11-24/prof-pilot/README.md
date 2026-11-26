# S1 profiling pilot (step 4 and site diagnosis)

bench/s1/run_workloads.sh prof evidence/s1-2026-11-24/prof-pilot w3-benchgen w2-wmtm w1-examples (RUNS=1), on
build/s1/cetta-prof (a9b3a811 + 0001-0005 + upstream/cetta-s1/0006, CETTA_FNPROF=1, -g; sha256 in ../binaries.txt),
CETTA_FNPROF_SAMPLE_US=1000. Per run: stdout (byte-identical to S0's base, .check), fnprof.json (S0's profile format),
samples.txt.gz (the sampler's raw output: per sample the profiler frame's function chain and a native backtrace, and a
copy of /proc/self/maps; sha256 of the uncompressed files in SAMPLES.sha256).

Recompute (needs the prof build for addr2line):
    python3 bench/s1/analyze_samples.py symbolise W/NAME.r1.samples.txt.gz W/NAME.r1.fnprof.json --binary build/s1/cetta-prof/cetta --out build/s1/sym/W.NAME.r1.sym.json
    python3 bench/s1/step4_search.py ... (docs/S1_HOT_SITES.md section 5)
