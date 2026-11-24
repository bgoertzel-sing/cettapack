#!/usr/bin/env python3
"""S1: symbolise and aggregate the C-level samples of upstream/cetta-s1/0006 (CETTA_FNPROF_SAMPLE_US).

Each sample: the profiler frame's function chain (leaf row first, then its ancestors; indexes into the
fnprof.json "functions" list) and a native backtrace.  The backtrace is symbolised against the binaries named in
the sample file's copy of /proc/self/maps: the cetta binary with addr2line -f -i (inlined chain; the build
carries -g), shared libraries with their symbol tables (nm).  Return addresses (every frame but the first) are
looked up at pc-1.

  analyze_samples.py symbolise SAMPLES FNPROF.json --out SYM.json     # one json per run: symbolised samples
  analyze_samples.py report SYM.json [SYM.json ...] [--rows ROWS] [--top N]

The report gives, per MeTTa row (self: the sample's leaf row) and over sets of rows (incl: any row on the chain),
the share of samples per native category (bench/s1/sample_categories.py) and the top native functions.
"""
import argparse
import bisect
import collections
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sample_categories  # noqa: E402

MAPS = re.compile(r"^M ([0-9a-f]+)-([0-9a-f]+) (\S+) ([0-9a-f]+) \S+ \d+\s*(.*)$")


def parse_samples(path):
    maps, samples, header = [], [], None
    import gzip
    opener = gzip.open if path.endswith(".gz") else open
    with opener(path, "rt") as f:
        for line in f:
            if line.startswith("#"):
                header = line.strip()
            elif line.startswith("M "):
                m = MAPS.match(line.rstrip("\n"))
                if m and "x" in m.group(3) and m.group(5).startswith("/"):
                    maps.append((int(m.group(1), 16), int(m.group(2), 16), int(m.group(4), 16), m.group(5)))
            elif line.startswith("S "):
                _, chain, pcs = line.rstrip("\n").split(" ", 2) if line.count(" ") >= 2 else (None, line.split()[1], "")
                samples.append(([int(x) for x in chain.split(",")], [int(x, 16) for x in pcs.split(",") if x]))
    maps.sort()
    return header, maps, samples


class NmSymbols:
    def __init__(self, path):
        out = []
        for flag in (["-D"], []):
            try:
                text = subprocess.run(["nm", "-n", "--defined-only"] + flag + [path], capture_output=True,
                                      text=True).stdout
            except OSError:
                text = ""
            for ln in text.splitlines():
                parts = ln.split()
                if len(parts) >= 3 and parts[1] in "TtWi":
                    out.append((int(parts[0], 16), parts[2]))
        out.sort()
        self.addrs = [a for a, _ in out]
        self.names = [n for _, n in out]

    def lookup(self, addr):
        i = bisect.bisect_right(self.addrs, addr) - 1
        return self.names[i] if i >= 0 else "?"


def elf_load_bias(path):
    """vaddr of the first PT_LOAD with offset 0 (0 for PIE)."""
    text = subprocess.run(["readelf", "-lW", path], capture_output=True, text=True).stdout
    for ln in text.splitlines():
        p = ln.split()
        if p and p[0] == "LOAD" and int(p[1], 16) == 0:
            return int(p[2], 16)
    return 0


def symbolise(args):
    header, maps, samples = parse_samples(args.samples)
    prof = json.load(open(args.fnprof))
    names = [fn["head"] for fn in prof["functions"]]
    starts = [m[0] for m in maps]
    cetta = os.path.realpath(args.binary) if args.binary else None

    def where(pc):
        i = bisect.bisect_right(starts, pc) - 1
        if i >= 0 and maps[i][0] <= pc < maps[i][1]:
            return maps[i]
        return None
    # unique (object, vaddr) per frame
    want = collections.defaultdict(set)
    framed = []
    for chain, pcs in samples:
        fr = []
        for d, pc in enumerate(pcs):
            look = pc if d == 0 else pc - 1
            m = where(look)
            if not m:
                fr.append(("?", 0))
                continue
            obj = m[3]
            vaddr = look - m[0] + m[2]
            fr.append((obj, vaddr))
            want[obj].add(vaddr)
        framed.append((chain, fr))
    resolved = {}
    for obj, addrs in want.items():
        addrs = sorted(addrs)
        is_cetta = (cetta and os.path.realpath(obj) == cetta) or os.path.basename(obj) == "cetta"
        if is_cetta:
            bias = elf_load_bias(obj)
            res = addr2line_inline(obj, [a + bias for a in addrs])
            for a, chain in zip(addrs, res):
                resolved[(obj, a)] = chain
        else:
            nm = NmSymbols(obj)
            base = os.path.basename(obj)
            for a in addrs:
                resolved[(obj, a)] = ["%s@%s" % (nm.lookup(a), base)]
    out = []
    for chain, fr in framed:
        stack = []
        for obj, a in fr:
            stack.append(resolved.get((obj, a), ["?"]))
        out.append({"rows": [names[c] if c < len(names) else "?%d" % c for c in chain], "stack": stack})
    json.dump({"header": header, "samples": out, "source": args.samples}, open(args.out, "w"))
    print("%s: %d samples symbolised -> %s" % (args.samples, len(out), args.out))


def addr2line_inline(obj, addrs):
    """For each address, the inlined chain innermost first (function names); addr2line -a starts each
    address' block with the address line, then one (function, file:line) pair per inlined level."""
    res = []
    for k in range(0, len(addrs), 20000):
        part = addrs[k:k + 20000]
        text = subprocess.run(["addr2line", "-a", "-f", "-i", "-e", obj] + ["%x" % a for a in part],
                              capture_output=True, text=True).stdout.splitlines()
        blocks, cur = [], None
        for ln in text:
            if ln.startswith("0x") and (cur is None or len(cur) % 2 == 0):
                if cur is not None:
                    blocks.append(cur)
                cur = []
            else:
                cur.append(ln)
        if cur is not None:
            blocks.append(cur)
        for b in blocks:
            fns = [b[i] for i in range(0, len(b), 2)]
            res.append([f for f in fns] or ["?"])
        while len(res) < k + len(part):
            res.append(["?"])
    return res


def report(args):
    total = 0
    per_row_cat = collections.defaultdict(collections.Counter)
    per_row_fn = collections.defaultdict(collections.Counter)
    all_cat = collections.Counter()
    row_sets = {}
    if args.rows:
        for spec in args.rows:
            name, _, members = spec.partition("=")
            row_sets[name] = set(members.split(","))
    set_cat = collections.defaultdict(collections.Counter)
    set_self = collections.defaultdict(collections.Counter)
    for path in args.sym:
        d = json.load(open(path))
        for s in d["samples"]:
            total += 1
            cat = sample_categories.categorise(s["stack"])
            leaf_fn = sample_categories.leaf_function(s["stack"])
            row = s["rows"][0]
            per_row_cat[row][cat] += 1
            per_row_fn[row][leaf_fn] += 1
            all_cat[cat] += 1
            for name, members in row_sets.items():
                if row in members:
                    set_self[name][cat] += 1
                if any(r in members for r in s["rows"]):
                    set_cat[name][cat] += 1
    res = {"total": total, "categories": dict(all_cat),
           "rows": {r: {"samples": sum(c.values()), "categories": dict(c),
                        "top_functions": per_row_fn[r].most_common(args.top)}
                    for r, c in per_row_cat.items()},
           "sets_self": {k: dict(v) for k, v in set_self.items()},
           "sets_incl": {k: dict(v) for k, v in set_cat.items()}}
    if args.json:
        json.dump(res, open(args.json, "w"), indent=1, sort_keys=True)
    print("samples: %d" % total)
    print("all samples by category:")
    for c, n in all_cat.most_common():
        print("  %-28s %6d  %5.1f%%" % (c, n, 100.0 * n / max(1, total)))
    ranked = sorted(per_row_cat.items(), key=lambda kv: -sum(kv[1].values()))[:args.top]
    for r, c in ranked:
        n = sum(c.values())
        print("\n%s  %d samples (%.1f%%)" % (r, n, 100.0 * n / max(1, total)))
        print("   " + ", ".join("%s %.0f%%" % (k, 100.0 * v / n) for k, v in c.most_common(8)))
        print("   top: " + ", ".join("%s %d" % (k, v) for k, v in per_row_fn[r].most_common(8)))
    for name in row_sets:
        for label, src in (("self", set_self), ("incl", set_cat)):
            c = src[name]
            n = sum(c.values())
            print("\nset %s (%s): %d samples (%.1f%%)" % (name, label, n, 100.0 * n / max(1, total)))
            print("   " + ", ".join("%s %.0f%%" % (k, 100.0 * v / max(1, n)) for k, v in c.most_common(12)))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("symbolise")
    s.add_argument("samples")
    s.add_argument("fnprof")
    s.add_argument("--binary")
    s.add_argument("--out", required=True)
    r = sub.add_parser("report")
    r.add_argument("sym", nargs="+")
    r.add_argument("--rows", action="append", help="NAME=row1,row2,... (a set of rows)")
    r.add_argument("--top", type=int, default=15)
    r.add_argument("--json")
    a = ap.parse_args()
    symbolise(a) if a.cmd == "symbolise" else report(a)


if __name__ == "__main__":
    main()
