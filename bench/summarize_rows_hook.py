#!/usr/bin/env python3
"""match-rows row hook (upstream/cetta 0004) against the embedding callback, every site.

    python3 bench/summarize_rows_hook.py DIR [DIR ...] > rows-hook.md

Reads runs-<site>.jsonl in each DIR (bench/run_fnspec.sh output) and, for every site with
both fn-spec (the compiled function as an embedding callback, cpk-fnspec:f) and fn-rows (the
same compiled code inside match-rows through the 0004 hook, cpk-rows:f), one row: the
median per-query time of each (min-max over processes), the paired ratio fn-spec / fn-rows
per run (> 1: the hook is faster; k/n runs where it was), the answers per batch of both
(they must be equal), peak RSS, and the path (both specialised every timed call).
make fnspec3-recompute regenerates the committed tables from the raw runs.
"""
import glob, json, os, statistics, sys
from collections import defaultdict

def load(d):
    out = {}
    for f in sorted(glob.glob(os.path.join(d, 'runs-*.jsonl'))):
        rows = defaultdict(list)
        for line in open(f):
            if line.startswith('{'):
                r = json.loads(line)
                rows[r['mode']].append(r)
        if 'fn-spec' in rows and 'fn-rows' in rows:
            out[os.path.basename(f)[5:-6]] = rows
    return out

def med(rs, key):
    v = [r[key] for r in rs]
    return statistics.median(v), min(v), max(v)

def paired(rows, ref, other):
    a = {r['run']: r['per_query_us_median'] for r in rows[ref]}
    b = {r['run']: r['per_query_us_median'] for r in rows[other]}
    rs = [a[k] / b[k] for k in sorted(a) if k in b and b[k]]
    return statistics.median(rs), min(rs), max(rs), sum(x > 1 for x in rs), len(rs)

def us(x):
    return '{:,.1f}'.format(x)

def main():
    print('# match-rows row hook (0004) vs embedding callback\n')
    print('fn-spec: the compiled function as an embedding callback (cpk-fnspec:f); fn-rows: the same generated code '
          'inside match-rows through the 0004 row hook (cpk-rows:f). Both run the same guards (program token / '
          'definition check, argument checks), the same row function (condition, template, per-answer normal-form '
          'check) and the same first-row cut. Paired: run i of fn-spec over run i of fn-rows; > 1 means the hook is '
          'faster.\n')
    allr, fails = [], []
    for d in sys.argv[1:]:
        sites = load(d)
        if not sites:
            continue
        print('## {}\n'.format(d))
        print('| site | shape | queries | fn-spec us | **fn-rows us** | **fn-rows vs fn-spec, paired** | answers per batch | '
              'peak RSS fn-rows / fn-spec MiB | path (all runs) |')
        print('|---|---|---|---|---|---|---|---|---|')
        for s, rows in sites.items():
            sp, rw = rows['fn-spec'], rows['fn-rows']
            x1 = med(sp, 'per_query_us_median')
            x2 = med(rw, 'per_query_us_median')
            p = paired(rows, 'fn-spec', 'fn-rows')
            allr.append((s, p[0]))
            a1 = {(r['answers_per_batch'], r['answer_items_first_batch']) for r in sp}
            a2 = {(r['answers_per_batch'], r['answer_items_first_batch']) for r in rw}
            k1 = {(r['true_answers_first_batch'], r['nonempty_tuples_first_batch']) for r in sp}
            k2 = {(r['true_answers_first_batch'], r['nonempty_tuples_first_batch']) for r in rw}
            same = a1 == a2 and len(a1) == 1 and k1 == k2
            if not same:
                fails.append('{}: answers differ between fn-spec {} {} and fn-rows {} {}'.format(s, a1, k1, a2, k2))
            c1, s1, r1 = (sum(r[k] for r in sp) for k in ('fn_calls', 'fn_spec_calls', 'fn_residual_calls'))
            c2, s2, r2 = (sum(r[k] for r in rw) for k in ('fn_calls', 'fn_spec_calls', 'fn_residual_calls'))
            if s1 != c1 or r1 or s2 != c2 or r2:
                fails.append('{}: residual calls (fn-spec {}/{}, fn-rows {}/{} specialised)'.format(s, s1, c1, s2, c2))
            bad = sum(r['bad_answers_first_batch'] for r in sp + rw)
            if bad:
                fails.append('{}: {} bad answers'.format(s, bad))
            a0 = sorted(a1)[0]
            rss_r = statistics.median(r['peak_rss_kb'] for r in rw) / 1024
            rss_s = statistics.median(r['peak_rss_kb'] for r in sp) / 1024
            print('| {} | {} | {} | {} ({}-{}) | **{}** ({}-{}) | **{:.2f}x** ({:.2f}-{:.2f}x, {}/{}) | {:,}{} | {:.1f} / {:.1f} | '
                  'fn-spec {:,}/{:,}, fn-rows {:,}/{:,} specialised |'.format(
                      s, sp[0]['shape'], sp[0]['queries'], us(x1[0]), us(x1[1]), us(x1[2]), us(x2[0]), us(x2[1]), us(x2[2]),
                      *p, a0[0], '' if same else ' DIFFER', rss_r, rss_s, s1, c1, s2, c2))
        print()
    if allr:
        v = sorted(x for _, x in allr)
        print('## Over all sites\n')
        print('- {} sites; paired fn-rows vs fn-spec, median over sites {:.2f}x, range {:.2f}x-{:.2f}x; faster on {} of {} sites '
              '(median ratio > 1)'.format(len(v), statistics.median(v), v[0], v[-1], sum(x > 1 for x in v), len(v)))
    print('\n## Checks\n')
    if fails:
        for f in fails:
            print('- FAIL ' + f)
    else:
        print('- every site: fn-spec and fn-rows give the same answers per batch (answer items, True answers, non-empty '
              'tuples); both specialised every timed call; no bad answers')
    print()

if __name__ == '__main__':
    main()
