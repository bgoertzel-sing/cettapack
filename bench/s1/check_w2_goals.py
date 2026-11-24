#!/usr/bin/env python3
"""w2-wmtm goal check (S1, fixing S0 review finding 1).

Reads a generated w2 program (bench/s0/gen_workloads.py w2-wmtm) and the stdout of runs of it, and reports, per
goal, what the construction implies and what the run answered:

  kind      pos / neg as the generator labelled it
  hops      inheritance steps from the instance to the goal class through the taxonomy (mem + inh statements);
            '-' when the class is not a taxonomy ancestor
  note      whether the goal class is reachable once the per-note Implication rules are added as edges
            (deductive closure of taxonomy + notes); a 'neg' goal reachable here is not a negative
  answered  the run's answer: '()' (no proof) or the strength/confidence of the first answer's STV

The query answers are the last N lines of stdout (one line per !(collapse (query ...)), in program order).

  check_w2_goals.py PROGRAM STDOUT [STDOUT ...] [--json OUT]
"""
import argparse
import json
import re
import sys

STMT = re.compile(r"^!\(compileadd \S+ \(: (\S+) (.*) \((STV|CTV) ")
GOAL = re.compile(r"^!\(collapse \(query (\d+) \S+ \(: \$prf \(Inheritance (\S+) (\S+)\) \$tv\)\)\)")
STV_END = re.compile(r"\(STV ([-0-9.e]+) ([-0-9.e]+)\)\)\)$")


def parse(program):
    inh, notes, goals = {}, [], []
    kind = None
    for line in open(program):
        line = line.rstrip("\n")
        if line.startswith(";; goal "):
            kind = line.split()[2]
            continue
        g = GOAL.match(line)
        if g:
            goals.append({"kind": kind, "steps": int(g.group(1)), "entity": g.group(2), "cls": g.group(3)})
            continue
        m = re.match(r"^!\(compileadd \S+ \(: (\S+) \(Inheritance (\S+) (\S+)\)", line)
        if m:
            inh.setdefault(m.group(2), []).append(m.group(3))
            continue
        m = re.match(r"^!\(compileadd \S+ \(: (\S+) \(Implication \(Inheritance \$x (\S+)\) \(Inheritance \$x (\S+)\)\)", line)
        if m:
            notes.append((m.group(2), m.group(3)))
    return inh, notes, goals


def hops(inh, src, dst, extra=()):
    edges = {k: list(v) for k, v in inh.items()}
    for a, b in extra:
        edges.setdefault(a, []).append(b)
    frontier, seen, d = [src], {src}, 0
    while frontier:
        if dst in frontier:
            return d
        nxt = []
        for n in frontier:
            for m in edges.get(n, []):
                if m not in seen:
                    seen.add(m)
                    nxt.append(m)
        frontier, d = nxt, d + 1
    return None


def answers(stdout, n):
    lines = open(stdout).read().rstrip("\n").split("\n")[-n:]
    out = []
    for ln in lines:
        if ln == "()":
            out.append("()")
        else:
            m = STV_END.search(ln)
            out.append("STV %.4g %.4g" % (float(m.group(1)), float(m.group(2))) if m else "answer")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("program")
    ap.add_argument("stdout", nargs="+")
    ap.add_argument("--json")
    a = ap.parse_args()
    inh, notes, goals = parse(a.program)
    runs = [answers(s, len(goals)) for s in a.stdout]
    identical = all(r == runs[0] for r in runs)
    rows = []
    for i, g in enumerate(goals):
        h = hops(inh, g["entity"], g["cls"])
        hn = hops(inh, g["entity"], g["cls"], notes)
        rows.append(dict(g, taxonomy_hops=h, with_notes_hops=hn, answer=runs[0][i]))
    print("| # | kind | goal | taxonomy hops | reachable with note rules | answer (run 1) |")
    print("|---|---|---|---|---|---|")
    for i, r in enumerate(rows, 1):
        print("| %d | %s | (Inheritance %s %s) | %s | %s | %s |" % (
            i, r["kind"], r["entity"], r["cls"], "-" if r["taxonomy_hops"] is None else r["taxonomy_hops"],
            "no" if r["with_notes_hops"] is None else "yes (%d)" % r["with_notes_hops"], r["answer"]))
    pos = [r for r in rows if r["kind"] == "pos"]
    neg = [r for r in rows if r["kind"] == "neg"]
    print()
    print("runs read: %d; answers identical across them: %s" % (len(runs), "yes" if identical else "NO"))
    print("positive goals with an answer: %d of %d" % (sum(r["answer"] != "()" for r in pos), len(pos)))
    print("negative goals with an answer: %d of %d" % (sum(r["answer"] != "()" for r in neg), len(neg)))
    print("negative goals deductively reachable (taxonomy + note rules): %d of %d" % (
        sum(r["with_notes_hops"] is not None for r in neg), len(neg)))
    if a.json:
        json.dump({"rows": rows, "identical": identical, "runs": len(runs)}, open(a.json, "w"), indent=1)
    return 0 if identical else 1


if __name__ == "__main__":
    sys.exit(main())
