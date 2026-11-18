#!/usr/bin/env python3
"""Insert the route probe into a copy of a cpk_host.c (both trees share
cmd_bench's shape). Usage: insert_probe.py SRC DST"""
import sys
src, dst = sys.argv[1], sys.argv[2]
t = open(src).read()
anchor_inc = "static int cmd_bench("
assert t.count(anchor_inc) == 1
t = t.replace(anchor_inc, '#include "route_probe.inc"\n\n' + anchor_inc)
a1 = "    uint64_t load_ns = cpk_now_ns() - t0;\n    free(kb);\n"
assert t.count(a1) == 1, "load anchor"
t = t.replace(a1, a1 + "    rp_backend(space, \"after-load\", mode);\n")
a2 = "    uint64_t *batch = calloc((size_t)repeat, sizeof(uint64_t));\n"
assert t.count(a2) == 1, "batch anchor"
t = t.replace(a2, "    rp_begin(e, space, mode);\n" + a2)
a3 = "    uint64_t sorted[1024];\n"
assert t.count(a3) == 1, "sorted anchor"
t = t.replace(a3, "    rp_end(e, space, mode);\n" + a3)
open(dst, "w").write(t)
