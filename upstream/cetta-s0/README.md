# upstream/cetta-s0: the S0 profiler patch (file only; not proposed to CeTTa)

`0005-fnprof-opt-in-PeTTa-function-profiler-CETTA_FNPROF-1.patch` applies on CeTTa a9b3a811 + upstream/cetta/0001-0004
(bench/s0/build_s0_cetta.sh on|off). It is kept out of upstream/cetta so the pinned series, its digest in every
pack's profile and make test stay as they were.

What it adds (all inside `#if CETTA_FNPROF`; `make CETTA_FNPROF=1`, default 0; the off build's machine code is the
uninstrumented build's, docs/S0_PROFILE.md section 2):

- src/cetta_fnprof.h: the flag and two hooks other files call.
- src/petta_search_machine.c: profiler frames carried by goals and choice points; frames opened by equation
  candidates, by prepared pure calls, by match starts (petta_machine_start_ready_match_mode) and by the existence
  fast paths over a match; self time between frame switches, inclusive time over the frame tree at exit; calls,
  candidates, results; &self equation/data revision changes between calls; report to $CETTA_FNPROF_OUT (JSON,
  schema cetta-fnprof-v1) at exit.
- src/eval.c petta_eval_machine_resolve_space: a space access by the current frame (name of the reference).
- src/space.c space_publish_mutation: a published mutation by the current frame (append-only or prefix-rewriting,
  equation projection changed or not).
- Makefile: `CETTA_FNPROF ?= 0`, `-DCETTA_FNPROF=1` and the build tag `.fnprof` when 1.

Report fields: docs/S0_PROFILE.md section 2. Consumers: bench/s0/classify_heads.py, bench/s0/summarize_s0.py.
