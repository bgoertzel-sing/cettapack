#!/usr/bin/env python3
"""The benchmark KB and queries of an fnspec site (made on first use).

    python3 bench/fnspec_inputs.py --site S --build build     # prints: --kb FILE --queries FILE (or nothing)
    python3 bench/fnspec_inputs.py --site S --cases-kind      # prints the bag-test case kind

The probe sites use the M1 KB (bench/gen_kb.py --seed 20261106, build/kb.metta) with the
probes' seeds per query kind (rs, gene, tissue with 40 queries); the real-code sites
use bench/gen_fnspec_kb.py; the test-only num sites have no benchmark KB.
"""
import argparse, filecmp, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def meta(site):
    m = {}
    for line in open(os.path.join(ROOT, 'fnspec', 'sites', site + '.metta')):
        g = re.match(r'\s*;;\s*([a-z-]+):\s*(.*?)\s*$', line)
        if g:
            m[g.group(1)] = g.group(2)
    return m

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--site', required=True)
    ap.add_argument('--build', default='build')
    ap.add_argument('--cases-kind', action='store_true')
    a = ap.parse_args()
    m = meta(a.site)
    kb, q = m.get('kb'), m.get('queries')
    if a.cases_kind:
        print(kb)
        return 0
    d = os.path.join(a.build, 'fnspec-kb')
    os.makedirs(d, exist_ok=True)
    if kb == 'm1':
        kbf = os.path.join(a.build, 'kb.metta')
        qf = os.path.join(d, 'queries-m1-%s.txt' % q)
        if not os.path.exists(qf):
            tmp = os.path.join(d, 'kb-m1-check.metta')
            extra = ['--queries', '40'] if q == 'tissue' else []
            subprocess.run([sys.executable, os.path.join(ROOT, 'bench', 'gen_kb.py'), '--query-kind', q, '--out', tmp,
                            '--queries-out', qf] + extra, check=True, stdout=subprocess.DEVNULL)
            if not filecmp.cmp(tmp, kbf, shallow=False):
                print('fnspec_inputs: the M1 KB differs from build/kb.metta', file=sys.stderr)
                return 1
            os.remove(tmp)
    elif kb in ('hm_live', 'mil', 'base_rate'):
        kbf = os.path.join(d, 'kb-%s.metta' % kb)
        qf = os.path.join(d, 'queries-%s.txt' % kb)
        if not (os.path.exists(kbf) and os.path.exists(qf)):
            subprocess.run([sys.executable, os.path.join(ROOT, 'bench', 'gen_fnspec_kb.py'), '--kind', kb, '--out', kbf,
                            '--queries-out', qf], check=True, stdout=subprocess.DEVNULL)
    else:
        return 0
    print('--kb %s --queries %s' % (kbf, qf))
    return 0

if __name__ == '__main__':
    sys.exit(main())
