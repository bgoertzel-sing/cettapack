#!/usr/bin/env python3
"""A.9 item 16: compare the kernel's answer bags with HE's on a KB HE can load.

  tests/he_diff.py --host build/cettapack-host --kb KB --queries Q

The host prints the kernel's bag per seed (after checking it equals generic
CeTTa's); HE runs the same match through hyperon. Bags are compared as sorted
multisets of answer texts."""
import argparse
import subprocess
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", required=True)
    ap.add_argument("--kb", required=True)
    ap.add_argument("--queries", required=True)
    a = ap.parse_args()
    try:
        from hyperon import MeTTa
    except ImportError:
        print("SKIP A.9-16: hyperon (HE) not installed")
        return 0
    out = subprocess.run([a.host, "bags", "--kb", a.kb, "--queries", a.queries],
                         capture_output=True, text=True)
    if out.returncode != 0:
        print("FAIL A.9-16 kernel vs HE: host bags failed:", out.stderr.strip())
        return 1
    kernel = {}
    for line in out.stdout.splitlines():
        seed, _, rest = line.partition("\t")
        kernel[seed] = sorted(x for x in rest.split(" | ") if x)
    m = MeTTa()
    space = m.space()
    for atom in m.parse_all(open(a.kb).read()):
        space.add_atom(atom)
    same = total = answers = 0
    for seed, kb in kernel.items():
        res = m.run(f"!(match &self (, (target {seed} $gene) (expressed $gene $tissue)) ($gene $tissue))")
        he = sorted(str(x) for x in res[0])
        total += 1
        answers += len(he)
        if he == kb:
            same += 1
        else:
            print(f"  differ at {seed}: kernel={kb} he={he}", file=sys.stderr)
    ok = same == total and total > 0
    import hyperon
    print(f"{'PASS' if ok else 'FAIL'} A.9-16 kernel vs HE {getattr(hyperon, '__version__', '?')}"
          f"  same_bag={same}/{total} answers={answers} kb={a.kb}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
