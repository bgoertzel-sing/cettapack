#!/usr/bin/env python3
"""HE (hyperon-experimental) on the same KB and site:
!(match &self (, (target RS $gene) (expressed $gene $tissue)) ($gene $tissue))
for the first --limit query seeds. Prints one JSON line."""
import argparse
import json
import resource
import statistics
import time

from hyperon import MeTTa


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kb", required=True)
    ap.add_argument("--queries", required=True)
    ap.add_argument("--limit", type=int, default=50)
    ap.add_argument("--repeat", type=int, default=3)
    a = ap.parse_args()
    metta = MeTTa()
    t0 = time.perf_counter()
    space = metta.space()
    with open(a.kb) as fh:
        text = fh.read()
    for atom in metta.parse_all(text):
        space.add_atom(atom)
    load_s = time.perf_counter() - t0
    qs = [q for q in open(a.queries).read().split() if q][: a.limit]
    batches, answers = [], 0
    for _ in range(a.repeat):
        b0 = time.perf_counter()
        n = 0
        for rs in qs:
            res = metta.run(f"!(match &self (, (target {rs} $gene) (expressed $gene $tissue)) ($gene $tissue))")
            n += len(res[0])
        batches.append(time.perf_counter() - b0)
        answers = n
    med = statistics.median(batches)
    import hyperon
    print(json.dumps({
        "mode": "he", "he_version": getattr(hyperon, "__version__", "0.2.10"),
        "kb_atoms": len(text.splitlines()), "queries": len(qs), "repeat": a.repeat,
        "answers_per_batch": answers, "batch_ns_median": int(med * 1e9),
        "per_query_us_median": med * 1e6 / len(qs), "kb_load_ms": load_s * 1e3,
        "peak_rss_kb": resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
    }))


if __name__ == "__main__":
    main()
