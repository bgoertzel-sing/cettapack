#!/usr/bin/env python3
"""Summarise bench/run_fnspec.sh output: one table per shape, one row per site.

    python3 bench/summarize_fnspec.py evidence/fnspec-2026-11-18/bench > summary.md

Per-query time: each process reports the median of its REPEAT batches; a cell is the
median over processes (min-max over processes in parentheses).  Paired: run i of the
reference over run i of fn-spec (they ran back to back); > 1 means fn-spec is faster;
"k/n" counts the runs where it was.  make fnspec-recompute runs this on the raw JSONL
and compares the result with the committed summary.
"""
import glob, json, os, statistics, sys
from collections import defaultdict

SHAPES = ['collapse-data', 'stream-data', 'collapse-filter', 'stream-filter',
          'exist-filter', 'once-data', 'exist-data', 'once-filter']
SHAPE_NOTE = {
    'collapse-data': 'survey rank 1',
    'stream-data': 'survey rank 2',
    'collapse-filter': 'survey rank 3',
    'stream-filter': "the probe's shape (survey rank 8), kept for the probe sites",
    'exist-filter': 'existence test + filter, survey rank 4',
    'once-data': 'collapse(once) + data, survey rank 5',
    'exist-data': 'existence test + data, survey rank 6',
    'once-filter': 'collapse(once) + filter, survey rank 7',
}

def load(d):
    sites = {}
    for f in sorted(glob.glob(os.path.join(d, 'runs-*.jsonl'))):
        rows = defaultdict(list)
        for line in open(f):
            line = line.strip()
            if line.startswith('{'):
                r = json.loads(line)
                rows[r['mode']].append(r)
        if rows:
            sites[os.path.basename(f)[5:-6]] = rows
    return sites

def med(rows, mode, key):
    v = [r[key] for r in rows[mode]]
    return statistics.median(v), min(v), max(v)

def paired(rows, ref, other):
    a = {r['run']: r['per_query_us_median'] for r in rows[ref]}
    b = {r['run']: r['per_query_us_median'] for r in rows[other]}
    rs = [a[k] / b[k] for k in sorted(a) if k in b and b[k]]
    return statistics.median(rs), min(rs), max(rs), sum(x > 1 for x in rs), len(rs)

def us(x):
    return '{:,.1f}'.format(x)

def route(rows):
    s = set()
    for m, rs in rows.items():
        for r in rs:
            b, a = r['route_before'], r['route_after']
            s.add((b['backend'], b['native_rows'], b['dense_facts'], a['native_rows'], a['dense_facts']))
    return '; '.join('{} rows+dense {:,}+{:,} -> {:,}+{:,}'.format(*k) for k in sorted(s))

def main():
    d = sys.argv[1]
    sites = load(d)
    print('# fnspec benchmark summary\n')
    print('Raw runs: runs-<site>.jsonl; environment: env.txt; inputs: inputs.txt; commands: commands.txt.\n')
    checks = []
    for shape in SHAPES:
        ss = [(s, rows) for s, rows in sites.items() if rows['fn-spec'][0]['shape'] == shape]
        if not ss:
            continue
        print('## {} ({})\n'.format(shape, SHAPE_NOTE[shape]))
        print('| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** '
              '| fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |')
        print('|---|---|---|---|---|---|---|---|---|---|---|')
        for s, rows in ss:
            cells = []
            for m in ('fn-generic', 'fn-hosted', 'fn-spec', 'fn-fallback'):
                x, lo, hi = med(rows, m, 'per_query_us_median')
                cells.append('{} ({}-{})'.format(us(x), us(lo), us(hi)) if m in ('fn-hosted', 'fn-spec') else us(x))
            ph = paired(rows, 'fn-hosted', 'fn-spec')
            pg = paired(rows, 'fn-generic', 'fn-spec')
            ans = {m: sorted({r['answers_per_batch'] for r in rows[m]}) for m in rows}
            items = {m: sorted({r['answer_items_first_batch'] for r in rows[m]}) for m in rows}
            same = len({tuple(v) for v in ans.values()}) == 1 and len({tuple(v) for v in items.values()}) == 1
            if not same:
                checks.append('{}: answers per batch differ between modes: {} {}'.format(s, ans, items))
            a0, i0 = ans['fn-spec'][0], items['fn-spec'][0]
            collapse = shape.startswith('collapse')
            anstxt = '{:,}'.format(a0) + (' tuples, {:,} items'.format(i0) if collapse else '') + ('' if same else ' DIFFER')
            if shape.startswith('once') or shape.startswith('exist'):
                key = 'nonempty_tuples_first_batch' if shape.startswith('once') else 'true_answers_first_batch'
                vals = {m: sorted({r[key] for r in rows[m]}) for m in rows}
                if len({tuple(v) for v in vals.values()}) != 1:
                    checks.append('{}: {} differ between modes: {}'.format(s, key, vals))
                    anstxt += ' DIFFER'
                n0 = vals['fn-spec'][0]
                anstxt = ('{:,} tuples, {:,} non-empty'.format(a0, n0) if shape.startswith('once')
                          else '{:,} answers, {:,} True'.format(a0, n0)) + ('' if same else ' DIFFER')
            rss_s = statistics.median(r['peak_rss_kb'] for r in rows['fn-spec']) / 1024
            rss_h = statistics.median(r['peak_rss_kb'] for r in rows['fn-hosted']) / 1024
            fs, fb = rows['fn-spec'], rows['fn-fallback']
            calls = sum(r['fn_calls'] for r in fs)
            spec = sum(r['fn_spec_calls'] for r in fs)
            res = sum(r['fn_residual_calls'] for r in fs)
            fbres, fbcalls = sum(r['fn_residual_calls'] for r in fb), sum(r['fn_calls'] for r in fb)
            if spec != calls or res:
                checks.append('{}: fn-spec took the residual on {} of {} calls'.format(s, res, calls))
            if fbres != fbcalls:
                checks.append('{}: fn-fallback specialised some calls'.format(s))
            print('| {} | {} | '.format(s, fs[0]['queries']) + ' | '.join(cells) +
                  ' | **{:.2f}x** ({:.2f}-{:.2f}x, {}/{}) | {:.2f}x ({:.2f}-{:.2f}x, {}/{})'.format(*ph, *pg) +
                  ' | {} | {:.1f} / {:.1f} | fn-spec {:,}/{:,} specialised; fn-fallback {:,}/{:,} residual |'.format(
                      anstxt, rss_s, rss_h, spec, calls, fbres, fbcalls))
        print()
        print('Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):\n')
        for s, rows in ss:
            print('- {}: {}'.format(s, route(rows)))
        print()
    # fnspec 2 cross-site check: an existence test and the collapse(once) of the same match on
    # the same queries must agree (True exactly where the first-row tuple is non-empty)
    pairs = [(k + '_exist_data', k + '_once_data') for k in ('te', 'gv', 'tf', 'vi')] + \
            [(k + '_exist_filter', k + '_once_filter') for k in ('te', 'gv', 'tf', 'vi')] + [('rs_sk_needed', 'rs_sk_needed_depth')]
    agree = []
    for e, o in pairs:
        if e in sites and o in sites:
            t = {r['true_answers_first_batch'] for rs in sites[e].values() for r in rs}
            n = {r['nonempty_tuples_first_batch'] for rs in sites[o].values() for r in rs}
            if t == n and len(t) == 1:
                agree.append('{} {:,} True = {} {:,} non-empty'.format(e, next(iter(t)), o, next(iter(n))))
            else:
                checks.append('{} True answers {} vs {} non-empty tuples {}'.format(e, sorted(t), o, sorted(n)))
    print('## Checks\n')
    for x in agree:
        print('- existence test agrees with collapse(once): ' + x)
    if checks:
        for c in checks:
            print('- FAIL ' + c)
    else:
        print('- every site: the same answers per batch (and answer items) in all four modes; fn-spec specialised every '
              'timed call; fn-fallback took the residual on every timed call')
    print()

if __name__ == '__main__':
    main()
