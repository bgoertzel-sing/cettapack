#!/usr/bin/env python3
"""Benchmark spaces for the real-code fnspec sites (t-2026-11-18-cettapack-fnspec).

    python3 bench/gen_fnspec_kb.py --kind hm_live|mil|base_rate --out KB --queries-out Q [--seed N]

Synthetic rows with the row shape of each site's pattern (the real programs' spaces are
filled at run time by PeTTaChainer / the MIL learner; their contents are not used):

  hm_live    ($hash $kb $consequent $name $antecedent $born), 120,000 rows over 2,000 KBs;
             queries: 200 KB ids
  mil        (MIL.FactPrimitive $program $relation) for 2,000 programs (1-3 relations each,
             from 500) and (MIL.Fact $relation $input $output), 100,000 rows; queries: 200
             (program input) pairs, half of them with facts
  base_rate  ($hash (base-rate $kbid $stored $tv $source $emode)), 100,000 rows, integer hashes
             0..4999, 50 KBs, statements from 2,000, modes any/pos/neg; queries: 200
             (hash kbid pattern mode), the pattern taken from the bucket's rows half of the time

About 5% of rows get a duplicate (multiplicity is real in MeTTa).
"""
import argparse, hashlib, random

def stmt(r, n):
    return '(Inheritance c%d c%d)' % (r.randrange(n), r.randrange(n))

def gen_hm(r):
    rows = []
    for i in range(120000):
        kb = 'kb%d' % r.randrange(2000)
        ant = stmt(r, 3000) if r.random() < 0.8 else '(And %s %s)' % (stmt(r, 3000), stmt(r, 3000))
        rows.append('(h%d %s %s hyp%d %s t%d)' % (r.randrange(50000), kb, stmt(r, 3000), i, ant, r.randrange(100)))
    qs = ['kb%d' % r.randrange(2000) for _ in range(200)]
    return rows, qs

def gen_mil(r):
    rows, prog_rel = [], {}
    for p in range(2000):
        rels = r.sample(range(500), r.randint(1, 3))
        prog_rel[p] = rels
        rows += ['(MIL.FactPrimitive p%d r%d)' % (p, x) for x in rels]
    rel_inputs = {}
    for _ in range(100000):
        rel, inp = r.randrange(500), r.randrange(1000)
        rel_inputs.setdefault(rel, []).append(inp)
        rows.append('(MIL.Fact r%d i%d o%d)' % (rel, inp, r.randrange(5000)))
    qs = []
    for k in range(200):
        p = r.randrange(2000)
        if k % 2 == 0:
            rel = r.choice(prog_rel[p])
            inp = r.choice(rel_inputs[rel]) if rel in rel_inputs else r.randrange(1000)
        else:
            inp = r.randrange(1000)
        qs.append('(p%d i%d)' % (p, inp))
    return rows, qs

def gen_br(r):
    rows, bucket = [], {}
    stmts = [stmt(r, 400) for _ in range(2000)]
    for _ in range(100000):
        h, kb, s = r.randrange(5000), 'kb%d' % r.randrange(50), r.choice(stmts)
        tv = '(stv %.2f %.2f)' % (r.random(), r.random())
        rows.append('(%d (base-rate %s %s %s s%d %s))' % (h, kb, s, tv, r.randrange(10), r.choice(['any', 'pos', 'neg'])))
        bucket.setdefault(h, []).append((kb, s))
    qs = []
    for k in range(200):
        h = r.randrange(5000)
        kb, s = r.choice(bucket[h]) if h in bucket else ('kb0', stmts[0])
        if k % 2:
            s = r.choice(stmts)
        qs.append('(%d %s %s %s)' % (h, kb, s, r.choice(['any', 'pos', 'neg'])))
    return rows, qs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--kind', required=True, choices=['hm_live', 'mil', 'base_rate'])
    ap.add_argument('--out', required=True)
    ap.add_argument('--queries-out', required=True)
    ap.add_argument('--seed', type=int, default=20261118)
    ap.add_argument('--dup-rate', type=float, default=0.05)
    a = ap.parse_args()
    r = random.Random(a.seed)
    rows, qs = {'hm_live': gen_hm, 'mil': gen_mil, 'base_rate': gen_br}[a.kind](r)
    out = []
    for x in rows:
        out.append(x)
        if r.random() < a.dup_rate:
            out.append(x)
    data = '\n'.join(out) + '\n'
    with open(a.out, 'w') as f:
        f.write(data)
    with open(a.queries_out, 'w') as f:
        f.write('\n'.join(qs) + '\n')
    print('%s: %d atoms, sha256 %s; %d queries' % (a.kind, len(out), hashlib.sha256(data.encode()).hexdigest(), len(qs)))

if __name__ == '__main__':
    main()
