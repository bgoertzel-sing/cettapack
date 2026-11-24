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

# fnspec 2 (t-2026-11-19-cettapack-fnspec-2): the real sites of the existence-test and
# collapse(once) shapes.  Row order matters for collapse(once): several rows per key, in
# an order where the first row is not the only one, and rows whose filter fails first.
GR = {
    'c1_rows': ("(r1 kb1)\n(r1 kb1)\n(r2 kb1)\n(r1 kb2)\n((ctx r3) kb1)\n(\"s\" kb2)\n(7 kb3)\n(kb1 kb1)\n",
                ['(kb1 r1)', '(kb2 r1)', '(kb1 r2)', '(kb2 r2)', '(kb1 (ctx r3))', '(kb2 "s")', '(kb3 7)', '(kb9 r9)',
                 '(kb1 kb1)', '(kb3 7.0)']),
    'c2_mixed': (";; expect: mixed\n($x kb4)\n(r5 $y)\n(r6 kb6)\n",
                 ['(kb4 r1)', '(kb7 r5)', '(kb4 $q)', '(kb6 r6)', '($k r6)']),
}
GR_NF = "!(add-atom &self (= true yes))\n"

LC = {
    'c1_rows': ("(compound-premise-mode And conj)\n(compound-premise-mode And conj2)\n(compound-premise-mode Or disj)\n"
                "(rule-form Implication BidirectionalImplication)\n(compound-premise-mode (Not x) neg)\n(logic-rule r1)\n"
                "(compound-premise-mode And conj)\n(compound-premise-mode 3 three)\n",
                ['And', 'Or', 'Not', 'Implication', '((Not x))', 'Zz', '"s"', '3', 'r1']),
    'c2_mixed': (";; expect: mixed\n(compound-premise-mode $h free)\n(compound-premise-mode Xor xor)\n",
                 ['And', 'Xor', '$q', '((Not $v))']),
}
LC_NF = "!(add-atom &self (= true yes))\n"

SK = {
    'c1_rows': ("(1 (sk a b) 0)\n(1 (sk a b) 0)\n(1 (sk a c) 2)\n(2 (sk a b) 5)\n(1 (sk a d) 9)\n(1 (sk a c) 4)\n"
                "(3 (sk (sk a) b) 6)\n(2 (sk a c) 8)\n(3 (sk a b) 7)\n",
                ['(1 (sk a b))', '(1 (sk a c))', '(2 (sk a b))', '(3 (sk a b))', '(1 (sk x y))', '(1 a)', '(2 (sk a c))',
                 '(3 (sk (sk a) b))', '(9 (sk a b))']),
    'c2_mixed': (";; expect: mixed\n(4 (sk $v b) 1)\n(4 (sk a b) 2)\n(5 1.5 0)\n(5 1 1)\n(6 \"s\" 2)\n(3 7 1)\n(3 (sk (sk a) b) 6)\n",
                 ['(4 (sk a b))', '(5 1)', '(4 (sk $q b))', '(6 "s")', '(5 1.5)', '(3 7)', '(3 (sk (sk a) b))']),
}
SK_NF = "!(add-atom &self (= true yes))\n!(add-atom &self (= (sk a c) skc))\n"

KP = {
    'c1_rows': ("(fact ctx1 p1 (stv 0.9 0.8))\n(fact ctx1 p1 (stv 0.5 0.5))\n(rule ctx2 p1 (stv 1.0 1.0))\n"
                "(fact ctx1 p2 (stv 0.1 0.2))\n(fact ctx1 (pf a) 3)\n(fact ctx1 p3 \"str\")\n(fact ctx1 p2 (stv 0.1 0.2))\n"
                "(rule ctx3 p4 (simple 1))\n(fact ctx4 p4 (stv 0.9 0.8))\n",
                ['p1', 'p2', 'p3', '((pf a))', 'p9', 'ctx1', 'p4']),
    'c2_mixed': (";; expect: mixed\n(fact ctx1 q1 $tv)\n(fact ctx1 q2 (stv $a 0.5))\n(fact ctx1 q3 (stv 0.9 0.8))\n",
                 ['q1', 'q2', 'q3', '$n']),
}
KP_NF = "!(add-atom &self (= (stv 0.9 $c) bumped))\n"

# fnspec 3 (t-2026-11-24-cettapack-fnspec-3): the real sites of the template-tree shapes.
# Several rows per key in an order where the first row is not the only one (collapse(once)),
# rows on every branch of the tree, and in the mixed cases values outside the compiled kinds.
HS = {
    'c1_rows': ("(1 kb1 (Inh a b) i1 ctx1)\n(1 kb1 (Inh a b) i2 ctx2)\n(1 kb1 (Inh a c) i1 ctx1)\n(2 kb1 (Inh a b) i1 ctx1)\n"
                "(1 kb2 (Inh a b) i3 ctx3)\n(1 kb1 (Inh a c) i4 ctx4)\n(2 kb1 (Inh a b) i1 ctx1)\n(3 kb1 (Sim a b) i5 ctx5)\n",
                ['(kb1 1 (Inh a b) i1 ctx1)', '(kb1 1 (Inh a b) i1 ctx9)', '(kb1 1 (Inh a b) i9 ctx1)', '(kb1 1 (Inh a b) i9 ctx9)',
                 '(kb1 1 (Inh a c) i4 ctx1)', '(kb1 2 (Inh a b) i1 ctx1)', '(kb2 1 (Inh a b) i3 ctx0)', '(kb9 1 (Inh a b) i1 ctx1)',
                 '(kb1 3 (Sim a b) i0 ctx0)', '(kb1 1 (Sim a b) i1 ctx1)']),
    'c2_mixed': (";; expect: mixed\n(4 kb1 (Inh $x b) i1 ctx1)\n(5 kb1 (Inh a b) 7 ctx1)\n(5 kb1 (Inh a b) i2 ctx2)\n(6 kb1 (Inh a b) 1.5 2)\n(7 kb1 (Inh a b) i1 ctx1)\n",
                 ['(kb1 4 (Inh a b) i1 ctx1)', '(kb1 5 (Inh a b) i1 ctx1)', '(kb1 5 (Inh a b) 7 ctx1)', '(kb1 6 (Inh a b) 1.5 2)',
                  '(kb1 6 (Inh a b) 1 2)', '(kb1 7 (Inh $q b) i1 ctx1)', '(kb1 7 (Inh a b) i1 ctx1)']),
}
HS_NF = "!(add-atom &self (= (Inh a c) inh-ac))\n"

SM = {
    'c1_rows': ("(1 (k a) 7 (pins p1) (cuts) (flt f1) v1)\n(1 (k a) 7 (pins p1) (cuts c1) (flt f1) v2)\n(1 (k a) 7 (pins p2) (cuts) (flt f1) v3)\n"
                "(1 (k b) 7 (pins p1) (cuts) (flt f1) v4)\n(1 (k a) 8 (pins p1) (cuts) (flt f2) (val v5))\n(1 (k a) 7 (pins p1) (cuts) (flt f1) v6)\n"
                "(2 (k a) 7 (pins p1) (cuts c1 c2) (flt f1) (val 3))\n(2 (k a) 7 (pins p1) (cuts) (flt f9) v7)\n(2 (k a) 7 (pins p1) (cuts) (flt f1) v8)\n",
                ['(1 (k a) 7 (pins p1) (cuts) (flt f1))', '(1 (k a) 7 (pins p1) (cuts c1) (flt f1))', '(1 (k a) 7 (pins p2) (cuts) (flt f1))',
                 '(1 (k b) 7 (pins p1) (cuts) (flt f1))', '(1 (k a) 8 (pins p1) (cuts) (flt f2))', '(1 (k a) 9 (pins p1) (cuts) (flt f1))',
                 '(2 (k a) 7 (pins p1) (cuts c1 c2) (flt f1))', '(2 (k a) 7 (pins p1) (cuts) (flt f1))', '(2 (k a) 7 (pins p1) (cuts) (flt f9))',
                 '(1 (k a) 7 (pins p3) (cuts) (flt f1))']),
    'c2_mixed': (";; expect: mixed\n(3 (k $v) 7 (pins p1) (cuts) (flt f1) v1)\n(3 (k a) 7 (pins p1) 3 (flt f1) v2)\n(3 (k a) 7 (pins p1) 3.0 (flt f1) v3)\n"
                 "(4 (k a) 7 (pins p1) (cuts) (flt f1) $free)\n(5 (k a) 7 (pins p1) (cuts) (flt f1) (v 1.5))\n",
                 ['(3 (k a) 7 (pins p1) 3 (flt f1))', '(3 (k a) 7 (pins p1) 3.0 (flt f1))', '(4 (k a) 7 (pins p1) (cuts) (flt f1))',
                  '(5 (k a) 7 (pins p1) (cuts) (flt f1))', '(3 (k $q) 7 (pins p1) (cuts) (flt f1))']),
}
SM_NF = "!(add-atom &self (= (val $x) (bumped $x)))\n"

SR = {
    'c1_rows': ("(1 (key a) 5 (sk-row id1 c p 0))\n(1 (key a) 5 (sk-row id2 c p 1))\n(1 (key b) 5 (sk-row id1 c p 2))\n"
                "(2 (key a) 5 (sk-row id1 c p 3))\n(1 (key a) 6 (sk-row id3 c p 4))\n(1 (key a) 5 (sk-row id1 c p 5))\n(1 (key a) 5 (sk-row (sid four) c p 6))\n",
                ['(1 (key a) 5 id1)', '(1 (key a) 5 id2)', '(1 (key a) 5 id3)', '(1 (key b) 5 id1)', '(1 (key a) 6 id3)', '(3 (key a) 5 id1)',
                 '(1 (key c) 5 id1)', '(1 (key a) 5 (sid four))', '(2 (key a) 5 id2)']),
    'c2_mixed': (";; expect: mixed\n(3 (key $x) 5 (sk-row id1 c p 0))\n(4 (key a) 5 (sk-row 7 c p 0))\n(4 (key a) 5 (sk-row id1 c p 1))\n(5 (key a) 5 (sk-row 1.5 c p 0))\n",
                 ['(3 (key a) 5 id1)', '(4 (key a) 5 id1)', '(4 (key a) 5 7)', '(5 (key a) 5 1.5)', '(5 (key a) 5 1)', '(3 (key $q) 5 id1)', '(6 (key a) 5 (sid 4))']),
}
SR_NF = "!(add-atom &self (= true yes))\n"

SB = {
    'c1_rows': ("(1 10 st (seg p1) (seg n1) (InstanceEvidence (BaseRateEvidence 0.5 0.0) (BaseRateEvidence 0.2 0.0)))\n"
                "(1 10 st (seg p2) (seg n2) (InstanceEvidence (BaseRateEvidence 0.5 0.3) (BaseRateEvidence 0.2 0.0)))\n"
                "(1 10 st (seg p3) (seg n3) (InstanceEvidence (BaseRateEvidence 0.5 0.0) (BaseRateEvidence 0.2 0.25)))\n"
                "(1 10 st (seg p2) (seg n2) (InstanceEvidence (BaseRateEvidence 0.5 0.3) (BaseRateEvidence 0.2 0.0)))\n"
                "(2 10 st (seg p5) (seg n5) (InstanceEvidence (BaseRateEvidence 1.0 0.0) (BaseRateEvidence 1.0 0.0)))\n"
                "(2 10 st (seg p6) (seg n6) (InstanceEvidence (BaseRateEvidence 2.0 0.0) (BaseRateEvidence 1.0 0.0)))\n"
                "(3 10 st (seg p7) (seg n7) other-contribution)\n(3 10 st (seg p8) (seg n8) (InstanceEvidence x y))\n"
                "(1 11 st (seg p9) (seg n9) (InstanceEvidence (BaseRateEvidence 0.1 0.0) (BaseRateEvidence 0.1 0.5)))\n",
                ['(1 10)', '(2 10)', '(3 10)', '(1 11)', '(9 9)', '(10 1)']),
    'c2_edges': ("(1 10 st (seg p3) (seg n3) (InstanceEvidence (BaseRateEvidence 0.5 -0.0) (BaseRateEvidence 0.2 0.0)))\n"
                 "(1 10 st (seg p4) (seg n4) (InstanceEvidence (BaseRateEvidence 0.5 0) (BaseRateEvidence 0.2 0)))\n"
                 "(2 10 st (seg p5) (seg n5) (InstanceEvidence (BaseRateEvidence 1 0.0) (BaseRateEvidence \"w\" 0.0)))\n"
                 "(3 10 st (seg p6) (seg n6) (InstanceEvidence (BaseRateEvidence 0.5 0.00) (BaseRateEvidence 0.2 0e0)))\n",
                 ['(1 10)', '(2 10)', '(3 10)']),
    'c3_mixed': (";; expect: mixed\n(1 10 st (seg p1) (seg n1) (InstanceEvidence (BaseRateEvidence 0.5 $w) (BaseRateEvidence 0.2 0.0)))\n"
                 "(1 10 st (seg p2) (seg n2) (InstanceEvidence (BaseRateEvidence 0.5 0.3) (BaseRateEvidence 0.2 0.0)))\n"
                 "(2 10 st (seg $s) (seg n5) (InstanceEvidence (BaseRateEvidence 0.5 0.3) (BaseRateEvidence 0.2 0.0)))\n"
                 "(3 10 st (seg p6) (seg n6) (InstanceEvidence (BaseRateEvidence 0.5 9223372036854775808) (BaseRateEvidence 0.2 0.0)))\n",
                 ['(1 10)', '(2 10)', '(3 10)', '($m 10)']),
}
SB_NF = "!(add-atom &self (= (seg p2) seg-p2))\n"

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
    write_kind(a.out, 'given_rules', GR, GR_NF)
    write_kind(a.out, 'logic_config', LC, LC_NF)
    write_kind(a.out, 'sk_needed', SK, SK_NF)
    write_kind(a.out, 'kb_proofs', KP, KP_NF)
    write_kind(a.out, 'hm_seen', HS, HS_NF)
    write_kind(a.out, 'sk_summary', SM, SM_NF)
    write_kind(a.out, 'sk_row', SR, SR_NF)
    write_kind(a.out, 'sk_member', SB, SB_NF)

if __name__ == '__main__':
    main()
