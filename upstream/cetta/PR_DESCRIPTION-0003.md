# PR: eval: match-rows, a direct entry to the conjunction matcher

Patch: `0003-eval-match-rows-direct-entry-to-the-conjunction-matcher.patch`
Applies to: zariuq/CeTTa a9b3a81186529fd99649e437809bdb6f58fce2fd after 0001 and 0002 (`git am`; it touches
neither of their files and also applies alone).
Size: 2 files, +35 (one X-macro symbol in src/symbol.h, one special form in src/eval.c).
Status: a file in this repository only; not pushed to CeTTa (task t-2026-11-18-cettapack-next-step-probe).

## What it adds

    (match-rows <space> <pattern> <row>)

<pattern> is one pattern or a flat conjunction (, p1 ... pn). CeTTa's conjunction matcher
(space_query_conjunction) runs on it, and for each binding row the call returns <row> with that row's bindings
applied, as data: no template is evaluated, nothing else is evaluated. Multiplicity is the matcher's (one row per
binding set). A non-space first argument gives the usual space-argument error. It is placed among the special forms,
so its arguments are not evaluated, like match's pattern and template.

## Why

Measured in docs/NEXT_STEP_PROBES.md (probe 1): called from MeTTa, match-rows answers the four benchmark sites
1.36-2.40x faster than the same matcher behind an embedding callback (hosted-generic) and 2.5-3.0x faster than
match, with the same result bags (bag tests on every seed). It is a narrow primitive for compiled hosts that already
know a site's rows are data.

## Semantics note

match evaluates each instantiated template; match-rows does not. The two agree only when the instantiated rows are in
normal form (no equation or grounded operation applies). A caller must establish that before replacing a match by
match-rows. No profile policy entry was added (the symbol is admitted wherever a builtin without a policy entry is).
