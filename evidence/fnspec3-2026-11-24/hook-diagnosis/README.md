Diagnostic of the match-rows hook, revision 1 vs revision 2 (docs/FNSPEC_3.md section 4.3). Not the benchmark method:
a nice'd process run beside the part 2 benchmark, 3 interleaved processes per variant, REPEAT=10.

- diag_prep.py: copies src/ and the generated plans to /tmp/f3diag and adds timing (inner_ns: enter..leave in the hook,
  compute() in the callback) and a fn-rows3 mode; run from the worktree at 96b3fa7.
- rev2_query_patch.py: the experiment that became 0004 revision 2 (hook-supplied query; the CeTTa part was then
  committed to 0004, the host part to src/cpk_fnspec.c at 0a3a591); the diag binary chose the query with CPK_ROWS_QUERY.
- compare.sh, compare-rev1-rev2.txt: per site and run, mode+query flag = per-query us / inside us per call / specialised
  calls / answers per batch (fn-spec0: callback; fn-rows0: hook on the call's instantiated pattern (rev 1);
  fn-rows1: hook with its own query (rev 2)). Scratch arena in both hook variants.
