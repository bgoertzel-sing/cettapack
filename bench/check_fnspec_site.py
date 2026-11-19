#!/usr/bin/env python3
"""Per-site structural check of fnspec benchmark runs (docs/EXPERIMENT_SANITY_GATE.md).

    python3 bench/check_fnspec_site.py runs-<site>.jsonl [--expect-answers N]

Refuses (exit 1) unless: every mode has the same answers per batch and answer items;
no answer of the first batch is an Error or an unevaluated call; fn-spec specialised every
timed call and fn-fallback took the residual on every timed call; with --expect-answers,
the answers per batch equal the known value (the probes' recorded counts).
"""
import json, sys
from collections import defaultdict

rows = defaultdict(list)
for line in open(sys.argv[1]):
    if line.startswith('{'):
        r = json.loads(line)
        rows[r['mode']].append(r)
errs = []
ans = {(r['answers_per_batch'], r['answer_items_first_batch']) for rs in rows.values() for r in rs}
if len(ans) != 1:
    errs.append('answers differ between modes/runs: %s' % sorted(ans))
for m, rs in rows.items():
    for r in rs:
        if r['bad_answers_first_batch']:
            errs.append('%s run %d: %d bad answers' % (m, r['run'], r['bad_answers_first_batch']))
for r in rows.get('fn-spec', []):
    if r['fn_spec_calls'] != r['fn_calls'] or r['fn_residual_calls']:
        errs.append('fn-spec run %d: %d/%d specialised' % (r['run'], r['fn_spec_calls'], r['fn_calls']))
for r in rows.get('fn-fallback', []):
    if r['fn_residual_calls'] != r['fn_calls'] or r['fn_spec_calls']:
        errs.append('fn-fallback run %d: residual %d/%d' % (r['run'], r['fn_residual_calls'], r['fn_calls']))
if '--expect-answers' in sys.argv:
    want = int(sys.argv[sys.argv.index('--expect-answers') + 1])
    got = {a for a, _ in ans}
    if got != {want}:
        errs.append('known-answer canary: answers per batch %s, expected %d' % (sorted(got), want))
if errs:
    print('CHECK FAILED %s:\n  ' % sys.argv[1] + '\n  '.join(errs))
    sys.exit(1)
print('check ok %s: %d modes x %d runs, answers per batch %s' % (sys.argv[1], len(rows), len(next(iter(rows.values()))), sorted(ans)))
