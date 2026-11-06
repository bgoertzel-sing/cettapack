#!/usr/bin/env python3
"""Seeded generator for the CettaPack M1 benchmark knowledge base.

The KB is a synthetic variant -> gene -> tissue graph:

    (target rsN geneM)        a variant targets a gene        (match site, factor 1)
    (expressed geneM tissueK) a gene is expressed in a tissue (match site, factor 2)
    (interacts geneA geneB)   noise relation
    (annotated geneM goK)     noise relation

`(target rsN $gene)` has a selective grounded prefix: a fixed rsN names one to
three genes among tens of thousands of target facts. A fraction of facts is
written more than once, so multiplicity matters under bag semantics.

Output: plain MeTTa atom text, one atom per line, plus a query-seed file.
The same seed and parameters give byte-identical output (SHA-256 printed).
"""
import argparse
import hashlib
import random
import sys


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=20261106)
    ap.add_argument("--snps", type=int, default=24000)
    ap.add_argument("--genes", type=int, default=6000)
    ap.add_argument("--tissues", type=int, default=48)
    ap.add_argument("--go-terms", type=int, default=2000)
    ap.add_argument("--interactions", type=int, default=30000)
    ap.add_argument("--annotations", type=int, default=20000)
    ap.add_argument("--dup-rate", type=float, default=0.06,
                    help="probability that a fact gets extra copies")
    ap.add_argument("--queries", type=int, default=200)
    ap.add_argument("--out", required=True)
    ap.add_argument("--queries-out", required=True)
    a = ap.parse_args(argv)

    rng = random.Random(a.seed)
    facts = []

    def emit(text):
        facts.append(text)
        if rng.random() < a.dup_rate:
            for _ in range(rng.choice((1, 1, 2))):
                facts.append(text)

    for s in range(a.snps):
        for g in rng.sample(range(a.genes), rng.choice((1, 1, 2, 3))):
            emit(f"(target rs{s} gene{g})")
    for g in range(a.genes):
        for t in rng.sample(range(a.tissues), rng.randint(2, 7)):
            emit(f"(expressed gene{g} tissue{t})")
    for _ in range(a.interactions):
        x, y = rng.randrange(a.genes), rng.randrange(a.genes)
        emit(f"(interacts gene{x} gene{y})")
    for _ in range(a.annotations):
        emit(f"(annotated gene{rng.randrange(a.genes)} go{rng.randrange(a.go_terms)})")

    rng.shuffle(facts)
    data = "".join(f + "\n" for f in facts).encode()
    with open(a.out, "wb") as fh:
        fh.write(data)

    # Query seeds: mostly present variants, 5% absent ones (the no-result path).
    qs = [f"rs{rng.randrange(a.snps)}" for _ in range(a.queries - a.queries // 20)]
    qs += [f"rs{a.snps + i}" for i in range(a.queries // 20)]
    rng.shuffle(qs)
    qdata = "".join(q + "\n" for q in qs).encode()
    with open(a.queries_out, "wb") as fh:
        fh.write(qdata)

    print(f"seed={a.seed} atoms={len(facts)} distinct={len(set(facts))} "
          f"kb_sha256={hashlib.sha256(data).hexdigest()} "
          f"queries={len(qs)} queries_sha256={hashlib.sha256(qdata).hexdigest()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
