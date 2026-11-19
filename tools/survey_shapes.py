#!/usr/bin/env python3
"""Static survey of MeTTa function-definition shapes (t-2026-11-18-cettapack-fnspec, step 2).

Counts every equation (= (head args...) body) in the .metta files of each corpus,
classifies its body by shape, and writes a table: shape, count per corpus, first
example location (file:line, relative to the corpus root).  No code text is copied
into the output.

    python3 tools/survey_shapes.py --corpus NAME=ROOT[:SUBDIR,SUBDIR...] ... \
        [--exclude NAME:GLOB ...] --json OUT.json --md OUT.md

Parsing: MeTTa s-expressions; ';' comments; "..." strings; a top-level form may be
preceded by '!'.  Only top-level (= LHS BODY) forms with an expression LHS headed by
a symbol count as function definitions (equations).  Identical files within a corpus
(same sha256) are counted once.
"""
import argparse, fnmatch, hashlib, json, os
from collections import Counter, OrderedDict, defaultdict

class Sym(str): pass
class Var(str): pass
class Str(str): pass

def tokenize(text):
    i, n, line = 0, len(text), 1
    while i < n:
        c = text[i]
        if c == '\n':
            line += 1; i += 1
        elif c.isspace():
            i += 1
        elif c == ';':
            while i < n and text[i] != '\n':
                i += 1
        elif c in '()':
            yield c, line; i += 1
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                if text[j] == '\\':
                    j += 1
                if j < n and text[j] == '\n':
                    line += 1
                j += 1
            yield Str(text[i + 1:j]), line
            i = j + 1
        else:
            j = i
            while j < n and not text[j].isspace() and text[j] not in '();"':
                j += 1
            tok = text[i:j]
            yield (Var(tok[1:]) if tok.startswith('$') else Sym(tok)), line
            i = j

def read_forms(text):
    """top-level forms as (form, line, bang)"""
    stack, out, bang, start = [], [], False, 0
    for tok, line in tokenize(text):
        if tok == '(' and not isinstance(tok, Str):
            if not stack:
                start = line
            stack.append([])
        elif tok == ')' and not isinstance(tok, Str):
            if not stack:
                continue
            e = tuple(stack.pop())
            if stack:
                stack[-1].append(e)
            else:
                out.append((e, start, bang)); bang = False
        else:
            if stack:
                stack[-1].append(tok)
            elif isinstance(tok, Sym) and tok.startswith('!'):
                bang = True
            else:
                bang = False
    return out

ARITH = {'+', '-', '*', '/', '%', 'mod', 'abs', 'min', 'max', 'pow-math', 'sqrt-math', 'abs-math',
         'floor-math', 'ceil-math', 'round-math', 'exp-math', 'log-math', 'trunc-math', 'min-atom', 'max-atom',
         'pow', 'sqrt', 'exp', 'log'}
CMP = {'==', '<', '>', '<=', '>=', '!=', '=?', '=alpha', 'not', 'and', 'or', 'xor'}
COND = {'if', 'case', 'switch', 'if-equal', 'if-decons', 'if-decons-expr', 'if-error', 'unify', 'if-unify'}
LETS = {'let', 'let*', 'chain', 'progn', 'sequential', 'function', 'return'}
COLLECT = {'collapse', 'collapse-bind', 'superpose', 'superpose-bind', 'foldall', 'forall', 'fold-flat',
           'once', 'limit', 'findall', 'unique', 'distinct', 'count', 'count-atoms', 'reduce', 'foldl-atom',
           'map-atom', 'filter-atom', 'foldl', 'map', 'filter', 'sum'}
LIST = {'car-atom', 'cdr-atom', 'cons-atom', 'decons-atom', 'size-atom', 'index-atom', 'union-atom',
        'intersection-atom', 'subtraction-atom', 'unique-atom', 'length', 'append', 'first', 'rest', 'cons',
        'nth', 'reverse', 'member'}
EFFECT = {'add-atom', 'remove-atom', 'println!', 'trace!', 'add-reduct', 'new-state', 'change-state!',
          'get-state', 'bind!', 'import!', 'pragma!', 'new-space', 'add-atoms', 'remove-all-atoms',
          'print', 'println', 'add-atom-nodup', 'set-state', 'write', 'transaction', 'test', 'assertEqual',
          'assertEqualToResult'}
EMPTY = {'empty'}
BUILTIN_CALLS = ARITH | CMP | COND | LETS | COLLECT | LIST | EFFECT | EMPTY | {
    'match', 'quote', 'eval', 'evalc', 'metta', 'get-type', 'get-metatype', 'repr', 'parse',
    'format-args', 'sealed', 'capture', 'nop', 'id', 'atom-subst', 'is-function', 'type-cast',
    'get-type-space', 'noreduce-eq', 'unquote', 'convert', 'string->symbol', 'symbol->string'}

def is_expr(x): return isinstance(x, tuple)
def head(x): return x[0] if is_expr(x) and x and isinstance(x[0], Sym) and not isinstance(x[0], Str) else None

def walk(x):
    yield x
    if is_expr(x):
        for c in x:
            yield from walk(c)

def heads_in(x): return {h for h in (head(s) for s in walk(x)) if h}

def evaluated_heads(x, defined):
    return {h for h in heads_in(x) if h in BUILTIN_CALLS or h in defined}

def is_data(x, defined): return not evaluated_heads(x, defined)
def only_ops(x, ops, defined): return evaluated_heads(x, defined) <= ops

def emp(z): return (head(z) == 'empty' and len(z) == 1) or (isinstance(z, Sym) and z == 'Empty')

def is_filter_if(t, defined):
    """(if COND (empty) T) or (if COND T (empty)): COND comparisons/arithmetic, T data"""
    if head(t) != 'if' or len(t) != 4:
        return False
    cond, a, b = t[1], t[2], t[3]
    if not (emp(a) or emp(b)):
        return False
    other = b if emp(a) else a
    return only_ops(cond, CMP | ARITH, defined) and is_data(other, defined)

def match_parts(b):
    return (b[1], b[2], b[3]) if head(b) == 'match' and len(b) == 4 else None

def classify(lhs, body, defined):
    fn = head(lhs)
    hs = heads_in(body)
    self_rec = fn in hs
    mp = match_parts(body)
    if mp:
        space, pat, tmpl = mp
        conj = head(pat) == ','
        depth, t = 1, tmpl
        while match_parts(t):
            depth += 1; t = match_parts(t)[2]
        if self_rec:
            return 'match: template recursive (calls the function itself)'
        if is_data(t, defined):
            return 'match: conjunctive/nested + data template' if (conj or depth > 1) else 'match: single pattern + data template'
        if is_filter_if(t, defined):
            return 'match + filter (if on bound values, empty branch)'
        if only_ops(t, ARITH | CMP, defined):
            return 'match + arithmetic/comparison on bound values'
        if only_ops(t, ARITH | CMP | COND | EMPTY | LETS, defined):
            return 'match + if/case/let on bound values (other)'
        return 'match + template calling other functions'
    if 'match' in hs:
        r = head(body)
        if r in COLLECT: return 'aggregate/collapse over a match'
        if r in COND: return 'if/case over a match'
        if r in LETS: return 'let/let* over a match'
        return 'other body containing a match'
    if hs & EFFECT:
        return 'side effects (no match)'
    if self_rec:
        if any(is_expr(a) for a in lhs[1:]) or hs & LIST:
            return 'recursion over a list/structure'
        return 'recursion (other)'
    if is_data(body, defined):
        return 'data expression (no evaluation)' if is_expr(body) else 'constant (symbol/variable/value)'
    r = head(body)
    if r in COND: return 'if/case conditional (no match)'
    if only_ops(body, ARITH | CMP, defined): return 'arithmetic/comparison expression'
    if r in LETS: return 'let/let* chain (no match)'
    if r in COLLECT: return 'aggregate/collapse/superpose (no match)'
    return 'call to other functions'


# ---- pass 2: every match occurrence inside a function body ---------------------
#
# The coarse body classes above put a match that sits under collapse, once, let,
# case or an existence test into "... over a match".  Pass 2 looks at each match
# itself: what its template does (template kind) and what the body does with its
# answers (context), and whether the whole body is nothing but that context around
# one match ("whole-function": the body has no evaluation outside the match and
# its wrapper).

def template_kind(t, defined):
    depth = 1
    while match_parts(t):
        depth += 1; t = match_parts(t)[2]
    hs = evaluated_heads(t, defined)
    if hs & EFFECT:
        k = 'effects'
    elif not hs:
        k = 'data'
    elif is_filter_if(t, defined):
        k = 'filter'
    elif hs <= ARITH | CMP:
        k = 'arithmetic/comparison'
    elif hs <= ARITH | CMP | COND | EMPTY | LETS:
        k = 'if/case/let'
    else:
        k = 'calls'
    return k, depth

def context_of(path):
    """path: ancestors of the match, nearest last, each (expr, index of child)"""
    if not path:
        return 'body'
    ctx = []
    for e, i in reversed(path):
        h = head(e)
        ctx.append((h, i, e))
    h0 = ctx[0][0]
    up = [c[0] for c in ctx]
    if h0 == 'once' and len(up) > 1 and up[1] == 'collapse':
        base, rest = 'collapse(once)', ctx[2:]
    elif h0 == 'collapse':
        base, rest = 'collapse', ctx[1:]
    elif h0 in ('foldall', 'fold-flat', 'fold-semidet', 'reduce', 'foldl-atom', 'count', 'count-atoms'):
        return 'fold/count'
    elif h0 == 'case' and ctx[0][1] == 1:
        return 'case scrutinee'
    elif h0 == 'once':
        return 'once'
    elif h0 == 'superpose':
        return 'superpose'
    else:
        base, rest = None, ctx
    if base:
        if rest and rest[0][0] == '==' and any(x == () for x in rest[0][2][1:]):
            return 'existence test (' + base + ' == ())'
        if rest and rest[0][0] == 'case' and rest[0][1] == 1:
            return base + ' as case scrutinee'
        if rest and rest[0][0] in ('let', 'let*'):
            return base + ' bound by let'
        if not rest:
            return base + ' as the body'
        return base + ' inside other code'
    if ctx[0][0] in ('let', 'let*'):
        return 'bound by let'
    if ctx[0][0] in COND:
        return 'under if/case'
    return 'other'

def match_occurrences(body):
    out = []
    def rec(x, path):
        if match_parts(x):
            out.append((x, list(path)))
            # do not descend into nested template matches (counted with the outer one)
            sp, pat, t = match_parts(x)
            for i, c in enumerate(x[1:3], 1):
                rec(c, path + [(x, i)])
            while match_parts(t):
                t = match_parts(t)[2]
            rec(t, path + [(x, 3)])
            return
        if is_expr(x):
            for i, c in enumerate(x):
                rec(c, path + [(x, i)])
    rec(body, [])
    return out

def whole_function(body, occ, defined):
    """the body is one match plus a wrapper that evaluates nothing else"""
    if len(occ) != 1:
        return None
    m, path = occ[0]
    for e, i in path:
        # every ancestor must be a wrapper form and its other arguments data
        h = head(e)
        if h not in ('collapse', 'once', '==', 'not', 'foldall', 'case', 'let', 'let*', 'if'):
            return None
        for j, c in enumerate(e[1:], 1):
            if j != i and evaluated_heads(c, defined):
                return None
    return context_of(path)


# ---- growth families: (what the body does with the answers) x (what the template does)

def wrapper_group(ctx):
    if ctx.startswith('existence test'):
        return 'existence test'
    if ctx.startswith('collapse(once)'):
        return 'collapse(once) (first answer)'
    if ctx.startswith('collapse'):
        return 'collapse (all answers as one tuple)'
    if ctx == 'fold/count':
        return 'fold/count'
    return 'stream (answers returned one by one)'

COMPILABLE_TEMPLATES = ('data', 'filter', 'arithmetic/comparison', 'if/case/let')

SHAPE_ORDER = [
    'match: single pattern + data template',
    'match: conjunctive/nested + data template',
    'match + filter (if on bound values, empty branch)',
    'match + arithmetic/comparison on bound values',
    'match + if/case/let on bound values (other)',
    'match + template calling other functions',
    'match: template recursive (calls the function itself)',
    'aggregate/collapse over a match',
    'if/case over a match',
    'let/let* over a match',
    'other body containing a match',
    'recursion over a list/structure',
    'recursion (other)',
    'if/case conditional (no match)',
    'arithmetic/comparison expression',
    'let/let* chain (no match)',
    'aggregate/collapse/superpose (no match)',
    'call to other functions',
    'data expression (no evaluation)',
    'constant (symbol/variable/value)',
    'side effects (no match)',
]

def corpus_files(root, subdirs, excludes):
    seen = set()
    for sd in subdirs or ['.']:
        base = os.path.join(root, sd)
        cands = []
        if os.path.isfile(base):
            cands = [base]
        else:
            for dp, dns, fns in os.walk(base):
                dns[:] = sorted(d for d in dns if d != '.git')
                cands += [os.path.join(dp, f) for f in sorted(fns) if f.endswith('.metta')]
        for p in cands:
            rel = os.path.normpath(os.path.relpath(p, root))
            if any(fnmatch.fnmatch(rel, g) for g in excludes) or rel in seen:
                continue
            seen.add(rel)
            yield rel

def survey(corpora, excl):
    counts = {n: Counter() for n in corpora}
    examples = {n: {} for n in corpora}
    info, per_eq = {}, []
    occ_counts = {n: Counter() for n in corpora}; occ_examples = {n: {} for n in corpora}
    wf_counts = {n: Counter() for n in corpora}; wf_examples = {n: {} for n in corpora}
    conj_counts = Counter()
    for name, (root, subs) in corpora.items():
        hashes, ndup, parsed = set(), 0, []
        for rel in corpus_files(root, subs, excl[name]):
            with open(os.path.join(root, rel), 'rb') as f:
                raw = f.read()
            h = hashlib.sha256(raw).hexdigest()
            if h in hashes:
                ndup += 1; continue
            hashes.add(h)
            parsed.append((rel, read_forms(raw.decode('utf-8', 'replace'))))
        defs = [(rel, form, line) for rel, forms in parsed for form, line, bang in forms
                if not bang and head(form) == '=' and len(form) == 3 and head(form[1])]
        defined = {head(f[1]) for _, f, _ in defs}
        for rel, form, line in defs:
            s = classify(form[1], form[2], defined)
            counts[name][s] += 1
            examples[name].setdefault(s, f'{rel}:{line}')
            occ = match_occurrences(form[2])
            wf = whole_function(form[2], occ, defined)
            mocc = []
            for m, path in occ:
                k, depth = template_kind(m[3], defined)
                c = context_of(path)
                conj = head(m[2]) == ',' or depth > 1
                mocc.append({'template': k, 'context': c, 'conjunctive_or_nested': conj})
                occ_counts[name][(c, k)] += 1
                occ_examples[name].setdefault((c, k), f'{rel}:{line}')
                if conj:
                    conj_counts[name] += 1
            if wf:
                k0 = mocc[0]['template']
                wf_counts[name][(wf, k0)] += 1
                wf_examples[name].setdefault((wf, k0), f'{rel}:{line}')
            per_eq.append({'corpus': name, 'loc': f'{rel}:{line}', 'head': head(form[1]), 'shape': s, 'matches': mocc, 'whole_function': wf})
        info[name] = {'root': root, 'subdirs': subs, 'excludes': excl[name], 'files': len(parsed),
                      'duplicate_files_skipped': ndup, 'equations': len(defs), 'function_heads': len(defined)}
    return counts, examples, info, per_eq, (occ_counts, occ_examples, wf_counts, wf_examples, conj_counts)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--corpus', action='append', required=True, help='NAME=ROOT[:SUB,SUB]')
    ap.add_argument('--exclude', action='append', default=[], help='NAME:GLOB (relative to the corpus root)')
    ap.add_argument('--json', required=True)
    ap.add_argument('--md', required=True)
    ap.add_argument('--per-equation', help='JSONL of every equation: corpus, file:line, head, shape')
    a = ap.parse_args()
    excl = defaultdict(list)
    for e in a.exclude:
        n, g = e.split(':', 1); excl[n].append(g)
    corpora = OrderedDict()
    for c in a.corpus:
        name, spec = c.split('=', 1)
        root, _, subs = spec.partition(':')
        corpora[name] = (root, [s for s in subs.split(',') if s])
    counts, examples, info, per_eq, extra = survey(corpora, excl)
    occ_counts, occ_examples, wf_counts, wf_examples, conj_counts = extra
    with open(a.json, 'w') as f:
        json.dump({'match_occurrences': {n: {f'{c} | {k}': v for (c, k), v in occ_counts[n].items()} for n in corpora},
                   'whole_function': {n: {f'{c} | {k}': v for (c, k), v in wf_counts[n].items()} for n in corpora},
                   'conjunctive_or_nested_matches': dict(conj_counts),
                   'corpora': info, 'counts': {n: dict(c) for n, c in counts.items()}, 'examples': examples,
                   'shape_order': SHAPE_ORDER}, f, indent=1, sort_keys=True)
    if a.per_equation:
        with open(a.per_equation, 'w') as f:
            for r in per_eq:
                f.write(json.dumps(r, sort_keys=True) + '\n')
    names = list(corpora)
    L = ['| shape | ' + ' | '.join(names) + ' | total | example location (first occurrence per corpus) |',
         '|---|' + '---|' * (len(names) + 2)]
    for s in SHAPE_ORDER:
        tot = sum(counts[n][s] for n in names)
        ex = '; '.join(f'{n}: {examples[n][s]}' for n in names if s in examples[n])
        L.append(f'| {s} | ' + ' | '.join(str(counts[n][s]) for n in names) + f' | {tot} | {ex} |')
    L.append('| **all equations** | ' + ' | '.join(str(info[n]['equations']) for n in names) +
             f" | {sum(info[n]['equations'] for n in names)} | |")
    L.append('| files (distinct) | ' + ' | '.join(str(info[n]['files']) for n in names) + ' | | |')
    def table(title, cnt, exs):
        keys = sorted({k for n in names for k in cnt[n]}, key=lambda k: -sum(cnt[n][k] for n in names))
        T = ['', title, '', '| context of the match | template | ' + ' | '.join(names) + ' | total | example location |',
             '|---|---|' + '---|' * (len(names) + 2)]
        for k in keys:
            tot = sum(cnt[n][k] for n in names)
            ex = '; '.join(f'{n}: {exs[n][k]}' for n in names if k in exs[n])
            T.append(f'| {k[0]} | {k[1]} | ' + ' | '.join(str(cnt[n][k]) for n in names) + f' | {tot} | {ex} |')
        T.append('| **all** | | ' + ' | '.join(str(sum(cnt[n].values())) for n in names) + f' | {sum(sum(cnt[n].values()) for n in names)} | |')
        return T
    L += table('Match occurrences inside function bodies (pass 2):', occ_counts, occ_examples)
    L.append('')
    L.append('Conjunctive or nested matches among them: ' + ', '.join(f'{n} {conj_counts[n]}' for n in names))
    fam_occ = {n: Counter() for n in names}; fam_wf = {n: Counter() for n in names}
    for (c, k), v in [(ck, v) for n in names for ck, v in occ_counts[n].items()]:
        pass
    for n in names:
        for (c, k), v in occ_counts[n].items():
            fam_occ[n][(wrapper_group(c), k)] += v
        for (c, k), v in wf_counts[n].items():
            fam_wf[n][(wrapper_group(c), k)] += v
    keys = sorted({k for n in names for k in fam_occ[n]}, key=lambda k: -sum(fam_occ[n][k] for n in names))
    L += ['', 'Growth families (pass 2 aggregated): what the body does with the answers x what the template does.',
          'compilable: the template evaluates only comparisons, arithmetic, if/case/let and empty on bound values;',
          'calls (other user functions) and effects are outside the fragment.', '',
          '| rank | answers | template | compilable | ' + ' | '.join(names) + ' | total occurrences | of them whole-function bodies |',
          '|---|---|---|---|' + '---|' * (len(names) + 2)]
    rank = 0
    for k in keys:
        comp = k[1] in COMPILABLE_TEMPLATES
        rank_s = ''
        if comp:
            rank += 1; rank_s = str(rank)
        tot = sum(fam_occ[n][k] for n in names); wtot = sum(fam_wf[n][k] for n in names)
        L.append(f'| {rank_s} | {k[0]} | {k[1]} | {"yes" if comp else "no"} | ' + ' | '.join(str(fam_occ[n][k]) for n in names) + f' | {tot} | {wtot} |')
    L += table('Whole-function bodies: one match plus a wrapper that evaluates nothing else (pass 2):', wf_counts, wf_examples)
    with open(a.md, 'w') as f:
        f.write('\n'.join(L) + '\n')
    print('\n'.join(L))

if __name__ == '__main__':
    main()
