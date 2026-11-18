#!/usr/bin/env python3
"""fnspecc: whole-function specialisation compiler (t-2026-11-18-cettapack-fnspec).

    python3 tools/fnspecc.py fnspec/sites/<site>.metta --out build/fnspec/<site>.c
    python3 tools/fnspecc.py --registry build/fnspec/registry.c <site.metta> ...

Reads one MeTTa function definition, (= (f $a1 ... $an) BODY), and compiles it to C
on CeTTa's matcher when BODY is one of the shapes chosen by the survey
(docs/FNSPEC_SURVEY.md):

  stream-data      (match S P T)                                T data
  collapse-data    (collapse (match S P T))                     T data
  collapse-filter  (collapse (match S P (if C T (empty))))      C comparisons on bound values
  stream-filter    (match S P (if C (empty) T))                 the probe's shape (kept for the probe sites)

(either if-branch may be the (empty) one).  S is an argument of f or a named space
token (&name); P is one pattern or a conjunction (, P1 ... Pn); in stream shapes a
template that is itself (match S P2 T2) on the same S is flattened into the
conjunction (P, P2) (the same bag; checked by the bag tests).  C is a tree of
and / or / not over (op X Y), op in == =alpha < > <= >=, X and Y variables or
ground constants.  Anything else is refused (exit status 3, a reason on stderr):
the function then stays CeTTa's.

Output: one C file with the site's plan (texts the runtime parses once, the shape,
the builtins the body relies on, with their arities, for the guard) and the
generated row function: the condition as C over the bound values, the template by
bindings_apply.  Guards, residual and the callable are src/cpk_fnspec.c.
"""
import argparse, hashlib, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from survey_shapes import read_forms, head, is_expr, Sym, Var, Str, walk

CMP_OPS = {'==': 'FS_EQ', '=alpha': 'FS_ALPHA', '<': 'FS_LT', '>': 'FS_GT', '<=': 'FS_LE', '>=': 'FS_GE'}
BOOL_OPS = {'and': 2, 'or': 2, 'not': 1}
EVALUATED = set(CMP_OPS) | set(BOOL_OPS) | {'if', 'case', 'let', 'let*', 'collapse', 'match', 'empty', 'superpose',
            '+', '-', '*', '/', '%', 'once', 'quote', 'eval', 'add-atom', 'remove-atom', 'println!', 'car-atom',
            'cdr-atom', 'cons-atom', 'unify', 'chain'}

class Refuse(Exception):
    pass

def show(x):
    if is_expr(x):
        return '(' + ' '.join(show(c) for c in x) + ')'
    if isinstance(x, Var):
        return '$' + x
    if isinstance(x, Str):
        return '"' + x.replace('\\', '\\\\').replace('"', '\\"') + '"'
    return str(x)

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'

def vars_of(x):
    out = []
    for s in walk(x):
        if isinstance(s, Var) and s not in out:
            out.append(s)
    return out

def is_data(x, defined):
    for s in walk(x):
        if is_expr(s) and s and isinstance(s[0], Sym) and not isinstance(s[0], Str):
            if s[0] in EVALUATED or s[0] in defined:
                return False
    return True

def is_empty_call(x):
    return is_expr(x) and len(x) == 1 and head(x) == 'empty'

def ground(x):
    return not vars_of(x)

def parse_site(path):
    text = open(path, encoding='utf-8').read()
    meta = {}
    for line in text.splitlines():
        m = re.match(r'\s*;;\s*([a-z-]+):\s*(.*?)\s*$', line)
        if m:
            meta[m.group(1)] = m.group(2)
    eqs = [f for f, line, bang in read_forms(text) if not bang and head(f) == '=' and len(f) == 3 and head(f[1])]
    return text, meta, eqs

def compile_site(path):
    text, meta, eqs = parse_site(path)
    name = meta.get('fnspec-site') or os.path.splitext(os.path.basename(path))[0]
    fname = meta.get('function')
    eq = [e for e in eqs if head(e[1]) == fname]
    if len(eq) != 1:
        raise Refuse(str(fname) + ': ' + str(len(eq)) + ' equations in the site file (one is compiled)')
    eq = eq[0]
    lhs, body = eq[1], eq[2]
    args = list(lhs[1:])
    if not all(isinstance(a, Var) for a in args) or len(set(args)) != len(args):
        raise Refuse('LHS arguments must be distinct variables')
    defined = {head(e[1]) for e in eqs}
    shape_wrap = 'stream'
    inner = body
    if head(body) == 'collapse' and len(body) == 2:
        shape_wrap, inner = 'collapse', body[1]
    if head(inner) != 'match' or len(inner) != 4:
        raise Refuse('body is not (match S P T) or (collapse (match S P T))')
    space, pat, tmpl = inner[1], inner[2], inner[3]
    pats = list(pat[1:]) if head(pat) == ',' else [pat]
    while head(tmpl) == 'match' and len(tmpl) == 4:
        if shape_wrap != 'stream':
            raise Refuse('nested match under collapse: row order of the flattened conjunction is not established')
        if tmpl[1] != space:
            raise Refuse('nested match on another space')
        p2 = tmpl[2]
        pats += list(p2[1:]) if head(p2) == ',' else [p2]
        tmpl = tmpl[3]
    if isinstance(space, Var):
        if space not in args:
            raise Refuse('match space is a variable that is not an argument')
        space_arg, space_token = args.index(space), None
    elif isinstance(space, Sym) and space.startswith('&'):
        space_arg, space_token = -1, str(space)
    else:
        raise Refuse('match space is neither an argument nor a named space token')
    for p in pats:
        if not is_data(p, defined) or not is_expr(p):
            raise Refuse('a pattern is not a data expression')
    cond, keep_when, out_tmpl = None, None, tmpl
    if head(tmpl) == 'if' and len(tmpl) == 4:
        c, a, b = tmpl[1], tmpl[2], tmpl[3]
        if is_empty_call(a) and is_data(b, defined):
            cond, keep_when, out_tmpl = c, 0, b
        elif is_empty_call(b) and is_data(a, defined):
            cond, keep_when, out_tmpl = c, 1, a
        else:
            raise Refuse('if template without an (empty) branch and a data branch')
    elif not is_data(tmpl, defined):
        raise Refuse('template evaluates something (not data, not a filter)')
    shape = shape_wrap + '-' + ('filter' if cond is not None else 'data')
    pat_vars = vars_of(tuple(pats))
    builtins = [('match', 3)]
    if shape_wrap == 'collapse':
        builtins.append(('collapse', 1))
    consts, cvars, leaves = [], [], []

    def operand(x):
        if isinstance(x, Var):
            if x == space:
                raise Refuse('the space is compared')
            if x in args:
                return 'FS_ARG(%d)' % args.index(x)
            if x not in pat_vars:
                raise Refuse('condition variable $' + x + ' is unbound')
            if x not in cvars:
                cvars.append(x)
            return 'FS_ROWVAR(%d)' % cvars.index(x)
        if is_expr(x) and not ground(x):
            raise Refuse('a compared constant has variables')
        if is_expr(x) and not is_data(x, defined):
            raise Refuse('a compared operand evaluates something')
        consts.append(x)
        return 'FS_CONST(%d)' % (len(consts) - 1)

    def comp(c):
        h = head(c)
        if h in CMP_OPS and len(c) == 3:
            if (h, 2) not in builtins:
                builtins.append((h, 2))
            i = len(leaves)
            x, y = operand(c[1]), operand(c[2])
            leaves.append('    int l%d = fs_compare(c, %s, %s, %s);\n'
                          '    if (l%d < 0) return -1;   /* (%s ...) outside the compiled value kinds */\n'
                          % (i, CMP_OPS[h], x, y, i, h))
            return 'l%d' % i
        if h in BOOL_OPS and len(c) == BOOL_OPS[h] + 1:
            if (h, BOOL_OPS[h]) not in builtins:
                builtins.append((h, BOOL_OPS[h]))
            parts = [comp(x) for x in c[1:]]
            if h == 'not':
                return '!' + parts[0]
            return '(' + (' && ' if h == 'and' else ' || ').join(parts) + ')'
        raise Refuse('condition form ' + show(c)[:60] + ' is outside the fragment')

    cond_c = None
    if cond is not None:
        builtins += [('if', 3), ('empty', 0)]
        expr = comp(cond)
        # every leaf is computed (and may abort) before the and/or combination
        cond_c = ''.join(leaves) + '    return (%s) ? %d : %d;\n' % (expr, keep_when, 1 - keep_when)
    hvars = [v for v in pat_vars if v not in args]
    seeds = [x for x in args if x != space]
    parts = '(_ {lhs} ({pats}) {tmpl} ({cvars}) ({consts}) ({hvars}) ({seeds}))'.format(
        lhs=show(lhs), pats=' '.join(show(p) for p in pats), tmpl=show(out_tmpl),
        cvars=' '.join('$' + v for v in cvars), consts=' '.join(show(k) for k in consts),
        hvars=' '.join('$' + v for v in hvars), seeds=' '.join('$' + v for v in seeds))
    equation = show(eq)
    hv = '(' + ' '.join('$' + v for v in hvars) + ')'
    hargs = ' '.join(show(x) for x in args)
    hosted_call = '(cpk-hosted:%s %s%s)' % (fname, (space_token + ' ') if space_token else '', hargs)
    inner_h = '(let %s %s %s)' % (hv, hosted_call, show(tmpl))
    hosted_body = ('(collapse %s)' % inner_h) if shape_wrap == 'collapse' else inner_h
    hosted_def = '(= (%s-h %s) %s)' % (fname, hargs, hosted_body)
    digest = hashlib.sha256(equation.encode()).hexdigest()
    return dict(name=name, fname=fname, arity=len(args), shape=shape, space_arg=space_arg, space_token=space_token,
                parts=parts, equation=equation, builtins=builtins, cond_c=cond_c, n_cvars=len(cvars),
                n_consts=len(consts), hosted_def=hosted_def, digest=digest, source=path, text=text, meta=meta,
                n_patterns=len(pats))

def emit(s):
    cid = re.sub(r'[^A-Za-z0-9_]', '_', s['name'])
    shape_c = {'stream-data': 'FS_STREAM_DATA', 'collapse-data': 'FS_COLLAPSE_DATA',
               'collapse-filter': 'FS_COLLAPSE_FILTER', 'stream-filter': 'FS_STREAM_FILTER'}[s['shape']]
    defs = '\n'.join(l for l in s['text'].splitlines() if l.strip() and not l.strip().startswith(';'))
    blist = ', '.join('%s/%d' % (b, k) for b, k in s['builtins'])
    out = []
    out.append('/* generated by tools/fnspecc.py from %s; do not edit.\n'
               ' * site %s: function %s/%d, shape %s, %d pattern(s)\n'
               ' * equation sha256 %s\n'
               ' * manifest: claim bag-tested-adequate; guards: the &self program token (the definition re-read\n'
               ' *   and compared whenever it moves), builtin meanings %s,\n'
               ' *   argument and row value kinds, answers in normal form; residual: CeTTa evaluates (%s ...) */\n'
               % (s['source'], s['name'], s['fname'], s['arity'], s['shape'], s['n_patterns'], s['digest'], blist, s['fname']))
    out.append('#include "cpk_fnspec.h"\n\n')
    if s['cond_c']:
        out.append('/* the condition, compiled: 1 keep the row, 0 drop it (the (empty) branch), -1 abort */\n'
                   'static int cond_%s(FsCall *c, Bindings *row, Arena *a) {\n%s}\n\n' % (cid, s['cond_c']))
    out.append('static int row_%s(FsCall *c, Bindings *row, Arena *a, Atom **answer) {\n' % cid)
    if s['cond_c']:
        out.append('    int k = cond_%s(c, row, a);\n    if (k <= 0) return k;\n' % cid)
    else:
        out.append('    /* data template: nothing to evaluate */\n')
    out.append('    *answer = bindings_apply(row, a, c->rt->tmpl);\n'
               '    if (!*answer || !fs_normal_form(c, *answer)) return -1;\n'
               '    return 1;\n}\n\n')
    b = ', '.join('{%s, %d}' % (cstr(n), k) for n, k in s['builtins'])
    out.append('const FsPlan fs_plan_%s = {\n' % cid)
    out.append('    .site = %s, .function = %s, .arity = %d,\n' % (cstr(s['name']), cstr(s['fname']), s['arity']))
    out.append('    .shape = %s, .space_arg = %d, .space_token = %s,\n'
               % (shape_c, s['space_arg'], cstr(s['space_token']) if s['space_token'] else 'NULL'))
    for k in ('parts', 'equation', 'hosted_def', 'digest'):
        out.append('    .%s = %s,\n' % (k, cstr(s[k])))
    out.append('    .definitions = %s,\n' % cstr(defs))
    out.append('    .builtins = {%s}, .n_builtins = %d,\n' % (b, len(s['builtins'])))
    out.append('    .n_cvars = %d, .n_consts = %d, .n_patterns = %d,\n' % (s['n_cvars'], s['n_consts'], s['n_patterns']))
    out.append('    .row = row_%s,\n' % cid)
    for k in ('kb', 'queries', 'origin', 'setup'):
        out.append('    .%s = %s,\n' % (k, cstr(s['meta'].get(k, ''))))
    out.append('};\n')
    return ''.join(out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('site', nargs='*')
    ap.add_argument('--out')
    ap.add_argument('--registry')
    a = ap.parse_args()
    if a.registry:
        ids = [re.sub(r'[^A-Za-z0-9_]', '_', os.path.splitext(os.path.basename(p))[0]) for p in a.site]
        with open(a.registry, 'w') as f:
            f.write('/* generated by tools/fnspecc.py --registry; do not edit */\n#include "cpk_fnspec.h"\n')
            for i in ids:
                f.write('extern const FsPlan fs_plan_%s;\n' % i)
            f.write('const FsPlan *const fs_plans[] = {' + ', '.join('&fs_plan_' + i for i in ids) + ', NULL};\n')
        return 0
    if len(a.site) != 1 or not a.out:
        ap.error('one site and --out')
    try:
        s = compile_site(a.site[0])
    except Refuse as e:
        print('fnspecc: %s: refused: %s' % (a.site[0], e), file=sys.stderr)
        return 3
    if s['name'] != os.path.splitext(os.path.basename(a.site[0]))[0]:
        print('fnspecc: the fnspec-site name must be the file name', file=sys.stderr)
        return 2
    os.makedirs(os.path.dirname(a.out) or '.', exist_ok=True)
    with open(a.out, 'w') as f:
        f.write(emit(s))
    print('fnspecc: %s: %s/%d -> %s (%d patterns, builtins guarded: %s) -> %s'
          % (s['name'], s['fname'], s['arity'], s['shape'], s['n_patterns'],
             ' '.join('%s/%d' % b for b in s['builtins']), a.out))
    return 0

if __name__ == '__main__':
    sys.exit(main())
