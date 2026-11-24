#!/usr/bin/env python3
"""Classify the profiled PeTTaChainer functions and match sites for S0 (t-2026-11-24-cettapack-s0-profile).

  python3 bench/s0/classify_heads.py --image IMAGE_DIR --profiles 'EVIDENCE/runs/on/*/*.fnprof.json' --out classification.json

The profiler reports two kinds of rows: functions (equation heads) and match sites ("match@F S (H ...)": the
space queries of function F on space S with pattern head H, timed in a frame of their own, so a function's self
time excludes its matches).  Equations are read from the image (PeTTaChainer subtree + PeTTa lib overlay;
examples, tests, benchmarks excluded) with tools/survey_shapes.py; the compiler is tools/fnspecc.py at this commit.

Functions:
  compiled   one equation and fnspecc accepts it (its fnspec shape); its match sites are compiled with it
  extension  it holds a match and fnspecc refuses it only for its form (several equations, patterns in the head):
             named extension, the whole function's self time counts toward it
  outside    no match (fnspec specialises space queries), or the code around a match that fnspecc refuses
             (let/if/calls around it: that code stays CeTTa's; the match itself is its own row)
Match sites:
  compiled   the enclosing function is compiled
  extension  "lift the match subexpression into a site": the match with its wrapper (collapse, once, existence
             test) compiles on its own (shape named); else the refusal of that subexpression, named
  outside    no enclosing function found in source (runtime-added equations, top level)
"""
import argparse
import glob
import json
import os
import re
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import survey_shapes as S  # noqa: E402
import fnspecc as F  # noqa: E402

# refusal text (substring) -> extension name
EXTENSIONS = [
    ("equations in the site file", "multi-equation functions (one compiled dispatch over several equations)"),
    ("LHS arguments must be distinct variables", "patterns in the function head (destructure before the match)"),
    ("body is not (match", "match inside a larger body"),
    ("nested match under collapse", "nested match under collapse (row order of the flattened conjunction)"),
    ("nested match on another space", "conjunction across two spaces"),
    ("match space is a variable that is not an argument", "space bound inside the body"),
    ("match space is neither", "space computed by an expression"),
    ("a pattern is not a data expression", "patterns that evaluate (calls inside the pattern)"),
    ("template node", "templates that call other functions or bind with let"),
    ("condition", "conditions outside the fragment (calls in if/case conditions)"),
    ("case", "case forms outside the fragment"),
    ("(== X Y)", "== tests other than existence tests"),
    ("(not ...)", "negations other than existence tests"),
    ("a compared", "comparisons with evaluated operands"),
    ("a boolean value", "boolean values over case-bound variables"),
    ("the space is compared", "the space compared as a value"),
    ("compiler error", "forms the compiler's reader does not take"),
]
WHOLE_FORM = ("equations in the site file", "LHS arguments must be distinct variables")
SITE_RE = re.compile(r"^(?:match@)+(\S+) .*?(\S+) \(([^()]*) \.\.\.\)$")


def ext_name(reason):
    for k, v in EXTENSIONS:
        if k in reason:
            return v
    return "other: " + reason[:80]


def has_match(form):
    return any(S.head(x) == "match" for x in S.walk(form))


def show(f):
    return F.show(f)


def try_compile(head, eq_text):
    with tempfile.NamedTemporaryFile("w", suffix=".metta", delete=False) as t:
        t.write(";; function: %s\n%s\n" % (head, eq_text))
        path = t.name
    try:
        return True, F.compile_site(path)["shape"]
    except F.Refuse as e:
        return False, str(e)
    except Exception as e:  # the reader's limits on unusual syntax
        return False, "compiler error: %s" % e
    finally:
        os.unlink(path)


def corpus_equations(image):
    eqs = {}
    for path in sorted(glob.glob(os.path.join(image, "**", "*.metta"), recursive=True)):
        rel = os.path.relpath(path, image)
        if rel.startswith(("examples/", "tests/", "benchmarks/", "docs/", "piPLN_paper_explained/", "linter/")):
            continue
        text = open(path, encoding="utf-8", errors="replace").read()
        try:
            forms = S.read_forms(text)
        except Exception:
            continue
        for f, line, bang in forms:
            if not bang and S.head(f) == "=" and len(f) == 3 and S.head(f[1]):
                eqs.setdefault(str(S.head(f[1])), []).append((rel + ":" + str(line), f))
    return eqs


def parents_map(form):
    par = {}

    def rec(x):
        if S.is_expr(x):
            for c in x:
                if S.is_expr(c):
                    par[id(c)] = x
                rec(c)
    rec(form)
    return par


def site_candidates(body, space, phead, any_head=False, var_space=False):
    """The match occurrences of the site, each with the wrappers fnspec takes (largest first)."""
    par = parents_map(body)
    out = []
    for x in S.walk(body):
        if S.head(x) != "match" or len(x) != 4:
            continue
        if var_space:
            if not isinstance(x[1], S.Var) or (phead == "_" and not isinstance(x[2], S.Var)):
                continue
        elif show(x[1]) != space:
            continue
        p = x[2]
        h = S.head(p)
        if h == ",":
            ph = "(...)"
        else:
            ph = str(h) if h else ("(...)" if S.is_expr(p) else "_")
        if not any_head and not var_space and ph != phead and not (phead == "(...)" and S.is_expr(p)):
            continue
        chain = [x]
        y = x
        while id(y) in par and S.head(par[id(y)]) in ("once", "collapse", "==", "not") and len(chain) < 4:
            y = par[id(y)]
            chain.append(y)
        out.append(list(reversed(chain)))
    return out


def classify_function(h, forms, entry):
    matchy = [f for f in forms if has_match(f[2])]
    if not matchy:
        entry.update({"class": "outside", "reason": "no match in the body: " + ", ".join(entry["survey_shapes"])})
        return
    if len(forms) == 1:
        ok, res = try_compile(h, show(forms[0]))
    else:
        ok, res = False, "%s: %d equations in the site file (one is compiled)" % (h, len(forms))
    if ok:
        entry.update({"class": "compiled", "shape": res})
    elif any(k in res for k in WHOLE_FORM):
        entry.update({"class": "extension", "refusal": res, "extension": ext_name(res)})
    else:
        entry.update({"class": "outside", "refusal": res,
                      "reason": "code around a match that fnspecc refuses (%s); the match is its own row" % ext_name(res)})


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True)
    ap.add_argument("--profiles", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    heads = {}
    for p in sorted(glob.glob(a.profiles)):
        for f in json.load(open(p))["functions"]:
            heads.setdefault(f["head"], f.get("first_equation", ""))
    eqs = corpus_equations(a.image)
    defined = set(eqs)
    out = {}

    def forms_of(h):
        if h in eqs:
            return [f for _, f in eqs[h]], [loc for loc, _ in eqs[h]], "source"
        return [], [], "not in source"

    for h in sorted(heads):
        if h == "<outside-equations>":
            out[h] = {"class": "none", "reason": "time outside any equation frame (loading, top-level evaluation)"}
            continue
        m = SITE_RE.match(h)
        if m:
            fn, space, phead = m.group(1), m.group(2), m.group(3)
            # The profiler names a site after the nearest enclosing equation frame.  A match inside a lambda
            # (|->), or in a helper CeTTa ran without a frame, belongs syntactically to another function:
            # look for the occurrence in fn first (pattern head as run, then any pattern on that space), then
            # in every function of the source.
            owners, how = [], ""
            for any_head in (False, True):
                if fn in eqs and any(site_candidates(f[2], space, phead, any_head) for _, f in eqs[fn]):
                    owners, how = [fn], "frame owner" + (" (pattern head bound at run time)" if any_head else "")
                    break
            var_space = False
            if not owners:
                tries = [(False, False)] + ([(False, True)] if phead == "_" else [(True, False)])
                for any_head, var_space in tries:
                    owners = sorted(g for g, lst in eqs.items()
                                    if any(site_candidates(f[2], space, phead, any_head, var_space) for _, f in lst))
                    if owners:
                        how = ("generic space helper (space and pattern are arguments)" if var_space else
                               "syntactic owner(s) elsewhere in the source (lambda or frameless helper)" + (
                                   ", pattern head bound at run time" if any_head else ""))
                        break
                else:
                    var_space = False
            entry = {"kind": "match site", "frame_function": fn, "space": space, "pattern_head": phead,
                     "owners": owners[:6], "owner_resolution": how}
            if not owners:
                entry.update({"class": "outside", "reason": "no match on %s found in the source (runtime-added equations or top level)" % space})
                out[h] = entry
                continue
            if var_space:
                entry.update({"class": "outside", "reason": "a whole-space scan through a generic helper (space and "
                              "pattern are variables, e.g. lib_spaces' clear/count over every atom): no fixed "
                              "pattern for fnspec to specialise"})
                out[h] = entry
                continue
            fn = owners[0]
            forms, locs, origin = forms_of(fn)
            entry["locations"] = locs[:6]
            any_head = "pattern head bound" in how
            pentry = {"survey_shapes": []}
            classify_function(fn, forms, pentry)
            if pentry["class"] == "compiled":
                entry.update({"class": "compiled", "shape": pentry["shape"] + " (in %s)" % fn})
                out[h] = entry
                continue
            if pentry["class"] == "extension":
                entry.update({"class": "extension", "extension": pentry["extension"], "refusal": pentry["refusal"]})
                out[h] = entry
                continue
            best, refusals = None, []
            for f in forms:
                for chain in site_candidates(f[2], space, phead, any_head, var_space):
                    for node in chain:
                        args = sorted({str(v) for v in S.walk(node) if isinstance(v, S.Var)})
                        lhs = "(%s-site %s)" % (fn, " ".join("$" + v for v in args))
                        ok, res = try_compile(fn + "-site", "(= %s %s)" % (lhs, show(node)))
                        if ok:
                            best = best or res
                            break
                        refusals.append(res)
            if best:
                entry.update({"class": "extension", "extension": "lift the match subexpression into a site",
                              "shape": best})
            elif refusals:
                entry.update({"class": "extension", "extension": ext_name(refusals[0]), "refusal": refusals[0]})
            else:
                entry.update({"class": "outside", "reason": "no match occurrence on %s with head %s found in %s" % (space, phead, fn)})
            out[h] = entry
            continue
        forms, locs, origin = forms_of(h)
        if not forms and heads[h]:
            try:
                forms = [f for f, _, _ in S.read_forms(heads[h]) if S.head(f) == "=" and len(f) == 3]
                origin = "runtime (first equation seen by the profiler)"
            except Exception:
                forms = []
        shapes = []
        for f in forms:
            try:
                shapes.append(S.classify(f[1], f[2], defined))
            except Exception as e:
                shapes.append("survey error: %s" % e)
        entry = {"kind": "function", "origin": origin, "locations": locs[:6], "equations": len(forms),
                 "survey_shapes": sorted(set(map(str, shapes)))}
        if not forms:
            entry.update({"class": "outside", "reason": "no equation found (builtin or host-provided head)"})
        else:
            classify_function(h, forms, entry)
        out[h] = entry
    json.dump(out, open(a.out, "w"), indent=1, sort_keys=True)
    print("classified %d rows -> %s" % (len(out), a.out))


if __name__ == "__main__":
    main()
