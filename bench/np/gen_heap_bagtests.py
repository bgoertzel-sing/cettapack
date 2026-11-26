#!/usr/bin/env python3
"""Heap bag tests for the native heap_pop primitive (upstream/cetta-np/0010).

The programs import PeTTaChainer as its examples do and use its heap functions: add_to_heap/empty_heap/heap_size
(answered by CeTTa's own native heap in every mode, for ground arguments) and heap_pop (heap_helpers.pl: SWI-Prolog
without the shim, CeTTa's native heap with it; heaps holding variables decline to SWI in both).

Each test is a PeTTa program (bench/np/bagtest/common.inc + generated cases).  A case is one sequence of heap
operations run as one let* chain: (add P X) -> add_to_heap, (pop) -> collapse(heap_pop) (an empty heap gives ()),
(size) -> heap_size.  The case prints every pop's answer (P X) or np-empty-pop, every size, and the final heap term
itself (its structure decides later tie order).  run_bagtests.sh runs each program with the shim off (SWI-Prolog's
heap_pop), on (native) and check (both, compared per call) and requires byte-identical stdout.

  gen_heap_bagtests.py OUTDIR [--seed N]
"""
import argparse
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))


def case(name, ops):
    binds, outs = ["($h0 (empty_heap))"], []
    h = 0
    for i, op in enumerate(ops):
        if op[0] == "add":
            binds.append("($h%d (add_to_heap $h%d %s %s))" % (h + 1, h, op[1], op[2]))
            h += 1
        elif op[0] == "pop":
            binds.append("($c%d (collapse (heap_pop $h%d)))" % (i, h))
            # pattern variables are unique per pop: one top-level expression shares its variables
            binds.append("($h%d (if (== $c%d ()) $h%d (let ($pa%d $xa%d $ra%d) (car-atom $c%d) $ra%d)))"
                         % (h + 1, i, h, i, i, i, i, i))
            binds.append("($o%d (if (== $c%d ()) np-empty-pop (let ($pb%d $xb%d $rb%d) (car-atom $c%d) (np-popped $pb%d $xb%d))))"
                         % (i, i, i, i, i, i, i, i))
            outs.append("$o%d" % i)
            h += 1
        elif op[0] == "size":
            binds.append("($o%d (np-size (heap_size $h%d)))" % (i, h))
            outs.append("$o%d" % i)
    return ("!(let* (%s)\n   (np-result %s (np-out %s) (np-final $h%d) (np-final-size (heap_size $h%d))))\n"
            % ("\n        ".join(binds), name, " ".join(outs), h, h))


def num(rng, kind):
    if kind == "int":
        return str(rng.randint(-5, 5))
    if kind == "float":
        return repr(rng.choice([-1.5, -0.25, 0.0, 0.25, 0.5, 1.0, 2.0, 0.123456789]))
    return rng.choice([str(rng.randint(-2, 2)), repr(float(rng.randint(-2, 2)))])  # int/float equal-value ties


ITEMS = ["(qitem a)", "(qitem b)", "(qgoal 1)", "(qgoal 2)", "(qagg 0.5 3)", "(qitem (Inheritance A B))",
         "(qitem (: p (Implication (Inheritance $x A) (Inheritance $x B)) (STV 0.9 0.8)))", "(qitem $v)",
         "(qitem \"s\")", "(qitem ())", "(qitem (a (b (c d))))", "(qitem 1.0)", "x"]


GROUND = [i for i in ITEMS if "$" not in i]


def rand_ops(rng, n, kind, pop_p, items=ITEMS):
    ops = []
    for _ in range(n):
        r = rng.random()
        if r < pop_p:
            ops.append(("pop",))
        elif r < pop_p + 0.05:
            ops.append(("size",))
        else:
            ops.append(("add", num(rng, kind), rng.choice(items)))
    ops += [("pop",)] * (n // 2 + 2) + [("size",)]
    return ops


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--seed", type=int, default=20261126)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    common = open(os.path.join(HERE, "bagtest", "common.inc")).read()
    tests = {}
    # hand cases
    ties = [("add", "1", "(qitem t%d)" % i) for i in range(7)] + [("pop",)] * 8
    dups = [("add", "2", "(qitem same)")] * 4 + [("add", "1", "(qitem same)")] * 2 + [("pop",)] * 7
    mixed = ([("add", "1", "(qitem int1)"), ("add", "1.0", "(qitem float1)"), ("add", "1", "(qitem int1b)"),
              ("add", "1.0", "(qitem float1b)"), ("add", "0.5", "(qitem half)"), ("add", "-0.0", "(qitem nz)"),
              ("add", "0", "(qitem zero)"), ("add", "0.0", "(qitem pz)")] + [("pop",)] * 9)
    empty = [("pop",), ("size",), ("pop",), ("add", "3", "(qitem a)"), ("pop",), ("pop",), ("size",)]
    inter = []
    for i in range(30):
        inter += [("add", str(i % 4), "(qitem i%d)" % i), ("add", str((i * 7) % 5), "(qitem j%d)" % i), ("pop",)]
    inter += [("pop",)] * 35 + [("size",)]
    neg = [("add", p, "(qitem n%s)" % p.replace("-", "m").replace(".", "_"))
           for p in ["-1", "-0.000001", "-1000000", "1e-6", "-2.5", "3"]] + [("pop",)] * 7
    tests["h1_hand"] = [("ties", ties), ("dups", dups), ("mixed", mixed), ("empty", empty), ("inter", inter),
                        ("neg", neg)]
    rng = random.Random(a.seed)
    tests["h2_random_small"] = [("r%d" % i, rand_ops(rng, rng.randint(5, 40), rng.choice(["int", "float", "mix"]),
                                                     rng.choice([0.1, 0.3, 0.5])))
                                for i in range(40)]
    tests["h3_random_ties"] = [("t%d" % i, rand_ops(rng, 60, "mix", 0.3)) for i in range(10)]
    big = [("add", num(rng, rng.choice(["int", "float", "mix"])), rng.choice(ITEMS)) for _ in range(800)]
    big += [("pop",)] * 300 + [("add", num(rng, "mix"), "(qitem late%d)" % i) for i in range(100)]
    big += [("pop",)] * 610 + [("size",)]
    tests["h4_large_with_vars"] = [("big", big)]
    # ground items only: CeTTa's native heap takes every call (a heap holding a variable declines to SWI)
    bigg = [("add", num(rng, rng.choice(["int", "float", "mix"])), rng.choice(GROUND)) for _ in range(800)]
    bigg += [("pop",)] * 300 + [("add", num(rng, "mix"), "(qitem late%d)" % i) for i in range(100)]
    bigg += [("pop",)] * 610 + [("size",)]
    tests["h5_large_ground"] = [("bigg", bigg)]
    tests["h6_random_ground"] = [("g%d" % i, rand_ops(rng, rng.randint(20, 120), rng.choice(["int", "float", "mix"]),
                                                      rng.choice([0.2, 0.4]), GROUND)) for i in range(20)]
    for name, cases in tests.items():
        with open(os.path.join(a.out, name + ".metta"), "w") as f:
            f.write(common)
            for cname, ops in cases:
                f.write(case(cname, ops))
    print(" ".join(sorted(tests)))


if __name__ == "__main__":
    main()
