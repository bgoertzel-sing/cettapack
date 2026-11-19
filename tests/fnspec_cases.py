#!/usr/bin/env python3
"""Small spaces for the fnspec bag tests (t-2026-11-18-cettapack-fnspec).

    python3 tests/fnspec_cases.py --out build/fnspec-cases

Writes <out>/<kind>/<case>.metta (facts; a line ';; expect: mixed' allows the residual on
some calls), <case>.q (one query per line: an argument, or a tuple of the non-space
arguments) and nf.mut (MeTTa run in &self by the "row-not-normal" mutation step: an
equation that rewrites some answers, so the compiled code must leave those calls to
the residual).  Kinds: m1 (the probes' six spaces), hm_live, mil, base_rate, num.
"""
import argparse, itertools, os, re

M1 = {
    'd1': "(target rs1 g1)\n(target rs1 g2)\n(target rs2 g3)\n(target rs9 g9)\n(expressed g1 liver)\n(expressed g1 brain)\n"
          "(expressed g2 heart)\n(expressed g3 skin)\n(interacts g1 g2)\n(interacts g3 g1)\n(annotated g1 go7)\n",
    'd2': "(target rs1 g1)\n(target rs1 g1)\n(target rs1 g1)\n(target rs1 g2)\n(target rs2 g1)\n(expressed g1 liver)\n"
          "(expressed g1 liver)\n(expressed g2 heart)\n(expressed g2 lung)\n(interacts g2 g1)\n(interacts g2 g1)\n(interacts g1 g1)\n",
    'd3': "(target rs1 g1)\n(target rs1 g2)\n(expressed g2 heart)\n(expressed g3 liver)\n(expressed g1 g1)\n"
          "(interacts g1 g2)\n(interacts g2 g2)\n(target g1 g1)\n",
    'd4': "(target rs1 g1)\n(target rs1 gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx)\n"
          "(expressed g1 liver)\n(expressed g1 liver)\n(expressed gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx heart)\n"
          "(interacts gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx g1)\n",
    'd5': "(target rs1 g1)\n(target rs1 g2)\n(target rs1 g2)\n(expressed g1 tissue0)\n(expressed g1 tissue0)\n(expressed g1 liver)\n"
          "(expressed g2 tissue0)\n(interacts g2 g1)\n(interacts g1 g2)\n(target tissue0 g1)\n(expressed tissue0 tissue0)\n(target rs3 tissue0)\n",
    'd6': "(target rs1 g1)\n(expressed g1 tissue0)\n(expressed g1 tissue0)\n(target rs2 g2)\n(target rs2 g3)\n"
          "(expressed g2 tissue0)\n(expressed g3 tissue0)\n(target rs4 g4)\n(expressed g4 tissue0)\n(target rs5 g5)\n(target rs5 g4)\n(expressed g5 kidney)\n"
          "(interacts g2 g1)\n(interacts g4 g4)\n",
}
M1_NF = "!(add-atom &self (= (g1 $t) (bumped $t)))\n!(add-atom &self (= (rs1 $t) (bumped $t)))\n!(add-atom &self (= (g3 $t) (bumped $t)))\n"

def m1_queries(text):
    seen = []
    for s in re.findall(r'[^\s()]+', text):
        if s not in seen:
            seen.append(s)
    return seen + ['absent_seed_zz']

HM = {
    'c1_rows': ("(h1 kb1 (Inh a b) n1 (Inh c d) t1)\n(h1 kb1 (Inh a b) n1 (Inh c d) t1)\n(h2 kb1 b n2 a t2)\n"
                "(h3 kb2 (Inh a b) n3 (And (Inh x y) (Inh y z)) t3)\n(h4 kb1 kb1 n4 kb1 t4)\n"
                "(h5 kb3 (Sim \"s\" 1) n5 (Inh 2.5 b) t5)\n(h6 kb1 (Inh c d) n6 (Inh a b) t6)\n",
                ['kb1', 'kb2', 'kb3', 'kb9', 'h1', 'n1', '((Inh a b))']),
    'c2_mixed': (";; expect: mixed\n(h7 kb4 (Inh $x b) n7 (Inh c d) t7)\n(h8 kb4 (Inh a b) n8 (Inh c d) t8)\n(h9 kb5 (Inh a b) n9 (Inh c d) t9)\n",
                 ['kb4', 'kb5', '((Inh $q b))', '(kb5)']),
}
HM_NF = "!(add-atom &self (= (Inh c d) cd-reduced))\n"

MIL_C1 = ("(MIL.FactPrimitive p1 r1)\n(MIL.FactPrimitive p1 r2)\n(MIL.FactPrimitive p2 r1)\n(MIL.FactPrimitive p1 r1)\n"
          "(MIL.FactPrimitive p3 p3)\n(MIL.Fact r1 a b)\n(MIL.Fact r1 a c)\n(MIL.Fact r2 a b)\n(MIL.Fact r1 b a)\n"
          "(MIL.Fact r1 a b)\n(MIL.Fact r3 a z)\n(MIL.Fact p3 p3 p3)\n(MIL.Fact r1 b (Pair a b))\n(MIL.Fact r2 b (Pair b b))\n")
MIL = {
    'c1_facts': (MIL_C1, ['(%s %s)' % (p, i) for p, i in itertools.product(['p1', 'p2', 'p3', 'p9'], ['a', 'b', 'p3', 'z9'])]),
    'c2_mixed': (";; expect: mixed\n(MIL.FactPrimitive q1 s1)\n(MIL.Fact s1 n 1)\n(MIL.Fact s1 n 1.5)\n(MIL.Fact s1 v $free)\n(MIL.Fact s1 w \"str\")\n",
                 ['(q1 n)', '(q1 v)', '(q1 w)', '(q1 $in)', '(q9 n)']),
}
MIL_NF = "!(add-atom &self (= (Pair a $y) paired))\n"

BR_C1 = ("(1 (base-rate kb1 (Inh a b) (stv 0.5 0.9) src1 any))\n(1 (base-rate kb1 (Inh a b) (stv 0.5 0.9) src1 any))\n"
         "(1 (base-rate kb1 (Inh a b) (stv 0.6 0.8) src2 pos))\n(1 (base-rate kb1 (Inh a c) (simple 0.1) src1 any))\n"
         "(1 (base-rate kb2 (Inh a b) (stv 0.5 0.9) src1 neg))\n(2 (base-rate kb1 (Inh a b) (stv 0.3 0.3) src3 neg))\n"
         "(1 (base-rate kb1 a (stv 1.0 1.0) src4 any))\n(1 (base-rate kb1 (Inh a b) (stv 0.2 0.2) src5 neg))\n")
BR = {
    'c1_rows': (BR_C1, ['(%s %s %s %s)' % t for t in itertools.product(['1', '2', '3'], ['kb1', 'kb2'],
                                                                       ['(Inh a b)', '(Inh a c)', 'a'], ['any', 'pos', 'neg'])]),
    'c2_mixed': (";; expect: mixed\n(1 (base-rate kb1 (Inh $x b) (stv 0.5 0.9) src1 any))\n(1 (base-rate kb1 (Inh a b) (stv 0.5 0.9) src1 7))\n"
                 "(1 (base-rate kb1 (Inh a b) (stv 0.5 0.9) src1 pos))\n",
                 ['(1 kb1 (Inh a b) pos)', '(1 kb1 (Inh $p b) any)', '(1 kb1 (Inh a b) 7)', '(1 kb1 (Inh a b) any)', '(1.0 kb1 (Inh a b) pos)']),
}
BR_NF = "!(add-atom &self (= (stv $s 0.9) (bumped $s)))\n"

I64MAX, I64MIN = '9223372036854775807', '-9223372036854775808'
NUM = {
    'c1_ints': ("(val k1 0 1)\n(val k1 5 10)\n(val k1 -1 -1)\n(val k1 %s %s)\n(val k1 %s 0)\n(val k2 3 2)\n(val k2 3 4)\n(val k2 3 4)\n"
                % (I64MAX, I64MAX, I64MIN),
                ['(%s %s)' % t for t in itertools.product(['k1', 'k2', 'k9'], ['0', '-1', '5', I64MAX, I64MIN, '3'])]),
    'c2_floats': ("(val f1 0.5 1.5)\n(val f1 -0.0 0.0)\n(val f1 123456789.25 123456789.25)\n(val f1 -98765.5 0.001)\n(val f1 1.5 1.0)\n(val f1 0.0 -0.0)\n",
                  ['(f1 %s)' % t for t in ['0.0', '-0.0', '0.5', '123456789.25', '-98765.5', '1.5']]),
    'c3_mixed': (";; expect: mixed\n(val m1 1 1.0)\n(val m1 1.0 1)\n(val m1 a 1)\n(val m1 9223372036854775808 1)\n(val m1 \"s\" 2)\n(val m1 True 1)\n(val m1 2 2)\n(val m1 1e308 1.0)\n(val m1 2.5e-300 1)\n",
                 ['(m1 %s)' % t for t in ['1', '1.0', 'a', '9223372036854775808', '"s"', '2', 'True', '1e308']]),
}
NUM_NF = "!(add-atom &self (= (k1 $x $y) bumped))\n!(add-atom &self (= (k2 $x $y) bumped))\n"

def write_kind(out, kind, cases, nf):
    d = os.path.join(out, kind)
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        os.remove(os.path.join(d, f))
    for name, (text, queries) in cases.items():
        with open(os.path.join(d, name + '.metta'), 'w') as f:
            f.write(text)
        with open(os.path.join(d, name + '.q'), 'w') as f:
            f.write('\n'.join(queries) + '\n')
    with open(os.path.join(d, 'nf.mut'), 'w') as f:
        f.write(nf)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    write_kind(a.out, 'm1', {k: (v, m1_queries(v)) for k, v in M1.items()}, M1_NF)
    write_kind(a.out, 'hm_live', HM, HM_NF)
    write_kind(a.out, 'mil', MIL, MIL_NF)
    write_kind(a.out, 'base_rate', BR, BR_NF)
    write_kind(a.out, 'num', NUM, NUM_NF)

if __name__ == '__main__':
    main()
