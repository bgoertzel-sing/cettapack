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
  once-data        (collapse (once (match S P T)))              collapse(once): the first answer, as a tuple
  once-filter      (collapse (once (match S P (if C T (empty)))))
  exist-data       (not (== (collapse [(once] (match S P T)[)]) ()))   existence test (fnspec 2); also
  exist-filter     the same with a filter template                     (== W ()) and (== () W)

(either if-branch may be the (empty) one).  collapse(once) and existence tests are the
survey's ranks 4-7 (docs/FNSPEC_SURVEY.md, Table 2); once is CeTTa's first-answer builtin
only in profiles that have it (extended, he-prime; in the plain he profile (once X) is data),
so a plan whose body uses once records that requirement and the runtime checks it.  S is an argument of f or a named space
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

def compile_tree(tmpl, args, pat_vars, defined, builtins, operand):
    """fnspec 3: C for a template tree (the survey's if/case/let family).

        T ::= (empty) | D | B | (if C T T) | (case X ((P1 T1) ... (Pn Tn)))
        C ::= (op X Y) | (and C C) | (or C C) | (not C) | True | False | (if C C C)
        B ::= a C used as the answer (he's True/False atoms)

    D is data (nothing evaluated); op is one of == =alpha < > <= >=, X and Y variables or
    ground constants; case's scrutinee is a variable (bound by the row or an argument), its
    patterns are data, and the variables a pattern binds may appear only in that branch's
    data leaves.  Returns (C code, template atoms: data leaves and case patterns, FS_TMPL(i),
    needs_bool).  A node computes only what CeTTa computes on its path (if: its condition,
    then one branch; case: the patterns in order until one matches); a value outside the
    compiled kinds aborts the call to the residual."""
    tmpls, helpers, protos, counter, needs_bool = [], [], [], [0], [False]

    def fresh(p):
        counter[0] += 1
        return '%s_%d' % (p, counter[0])

    def need(b):
        if b not in builtins:
            builtins.append(b)

    def cond_fn(c):
        leaves = []

        def comp(c):
            h = head(c)
            if isinstance(c, Sym) and not isinstance(c, Str) and str(c) in ('True', 'False'):
                return '1' if str(c) == 'True' else '0'
            if h in CMP_OPS and len(c) == 3:
                need((h, 2))
                i = len(leaves)
                x, y = operand(c[1]), operand(c[2])
                leaves.append('    int l%d = fs_compare(c, %s, %s, %s);\n'
                              '    if (l%d < 0) return -1;   /* (%s ...) outside the compiled value kinds */\n'
                              % (i, CMP_OPS[h], x, y, i, h))
                return 'l%d' % i
            if h in BOOL_OPS and len(c) == BOOL_OPS[h] + 1:
                need((h, BOOL_OPS[h]))
                parts = [comp(x) for x in c[1:]]
                if h == 'not':
                    return '!' + parts[0]
                return '(' + (' && ' if h == 'and' else ' || ').join(parts) + ')'
            if h == 'if' and len(c) == 4:
                need(('if', 3))
                return '(%s ? %s : %s)' % (comp(c[1]), comp(c[2]), comp(c[3]))
            if isinstance(c, Sym) and not isinstance(c, Str):
                raise Refuse('condition leaf %s is not a Bool (if is (-> Bool Atom Atom $t) in he)' % show(c))
            raise Refuse('condition form ' + show(c)[:60] + ' is outside the fragment')

        expr = comp(c)
        name = fresh('cond')
        # every leaf of this condition is computed (and may abort) before the combination
        helpers.append('static int %s(FsCall *c, Bindings *row, Arena *a) {\n    (void)row; (void)a;\n%s    return (%s) ? 1 : 0;\n}\n'
                       % (name, ''.join(leaves), expr))
        return name

    def is_bool_expr(t):
        h = head(t)
        return (h in CMP_OPS and len(t) == 3) or (h in BOOL_OPS and len(t) == BOOL_OPS[h] + 1)

    def node(t, case_vars):
        """C statements that return 1 (an answer), 0 (no answer: (empty)), 2 (no answer: no case
        branch matches), -1 (abort)"""
        if is_empty_call(t):
            need(('empty', 0))
            return '    return 0;\n'
        h = head(t)
        if h == 'if' and len(t) == 4:
            need(('if', 3))
            if any(v in case_vars for v in vars_of(t[1])):
                raise Refuse('a condition uses a variable bound by a case pattern')
            cn = cond_fn(t[1])
            return ('    { int k = %s(c, row, a); if (k < 0) return -1;\n      if (k) {\n%s      } else {\n%s      } }\n'
                    % (cn, node(t[2], case_vars), node(t[3], case_vars)))
        if h == 'case' and len(t) == 3:
            if case_vars:
                raise Refuse('case inside a case branch')
            x, branches = t[1], t[2]
            if not isinstance(x, Var):
                raise Refuse('case scrutinee is not a variable')
            if not is_expr(branches) or not branches or not all(is_expr(b) and len(b) == 2 for b in branches):
                raise Refuse('case branches are not (pattern body) pairs')
            need(('case', 2))
            sv = operand(x)
            out = ['    { Atom *sv = %s;\n      if (!sv || !fs_normal_form(c, sv)) return -1;   /* the scrutinee is a value */\n' % sv]
            for p, body in branches:
                if not is_data(p, defined):
                    raise Refuse('a case pattern evaluates something')
                bound = [v for v in vars_of(p) if v not in pat_vars and v not in args]
                bn = fresh('branch')
                protos.append('static int %s(FsCall *c, Bindings *row, Arena *a, Atom **answer, Bindings *cb);\n' % bn)
                helpers.append('static int %s(FsCall *c, Bindings *row, Arena *a, Atom **answer, Bindings *cb) {\n'
                               '    (void)cb;\n%s}\n' % (bn, node(body, set(bound) | {'.case'})))
                tmpls.append(p)
                k = len(tmpls) - 1
                out.append('      { Bindings cb; bindings_init(&cb);\n'
                           '        Atom *pt = FS_TMPLV(%d);   /* the pattern under the row */\n'
                           '        if (!pt) { bindings_free(&cb); return -1; }\n'
                           '        if (simple_match(pt, sv, &cb, a)) { int r = %s(c, row, a, answer, &cb); bindings_free(&cb); return r; }\n'
                           '        bindings_free(&cb); }\n' % (k, bn))
            out.append('      return 2; }\n')
            return ''.join(out)
        if is_bool_expr(t):
            if any(v in case_vars for v in vars_of(t)):
                raise Refuse('a boolean value uses a variable bound by a case pattern')
            needs_bool[0] = True
            cn = cond_fn(t)
            return ('    { int k = %s(c, row, a); if (k < 0) return -1;\n'
                    '      *answer = k ? FS_TRUE : FS_FALSE; return *answer ? 1 : -1; }\n' % cn)
        if is_data(t, defined):
            tmpls.append(t)
            k = len(tmpls) - 1
            app = '    *answer = FS_TMPLV(%d);\n' % k
            if case_vars:
                app += '    if (*answer && cb) *answer = bindings_apply(cb, a, *answer);\n'
            return app + '    if (!*answer || !fs_normal_form(c, *answer)) return -1;\n    return 1;\n'
        raise Refuse('template node ' + show(t)[:60] + ' is outside the fragment (if, case, empty, data, boolean)')

    top = node(tmpl, set())
    code = ''.join(protos) + '\n' + '\n'.join(helpers)
    code += '\n/* the template tree: 1 an answer, 0 (empty), 2 no case branch, -1 abort */\n'
    code += 'static int tree_top(FsCall *c, Bindings *row, Arena *a, Atom **answer) {\n    Bindings *cb = NULL; (void)cb;\n%s}\n' % top
    return code, tmpls, needs_bool[0]


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
    once, exist, negated = False, 0, False
    b = body
    # existence test: (not (== W ())), (== W ()), either operand order; W a collapse
    if head(b) == 'not' and len(b) == 2 and head(b[1]) == '==':
        negated, b = True, b[1]
    if head(b) == '==' and len(b) == 3:
        if b[2] == () and head(b[1]) == 'collapse':
            b = b[1]
        elif b[1] == () and head(b[2]) == 'collapse':
            b = b[2]
        else:
            raise Refuse('(== X Y) body that is not an existence test (collapse ...) against ()')
        exist = 1 if negated else 2          # 1: True when some row survives; 2: True when none
    elif negated:
        raise Refuse('(not ...) body that is not an existence test')
    inner = b
    if head(b) == 'collapse' and len(b) == 2:
        shape_wrap, inner = 'collapse', b[1]
        if head(inner) == 'once' and len(inner) == 2:
            once, inner = True, inner[1]
    if head(inner) != 'match' or len(inner) != 4:
        raise Refuse('body is not (match S P T), (collapse (match S P T)), (collapse (once (match S P T))) or an existence test of them')
    match_node = inner
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
    cond, keep_when, out_tmpl, tree = None, None, tmpl, False
    if head(tmpl) == 'if' and len(tmpl) == 4 and is_empty_call(tmpl[2]) and is_data(tmpl[3], defined):
        cond, keep_when, out_tmpl = tmpl[1], 0, tmpl[3]
    elif head(tmpl) == 'if' and len(tmpl) == 4 and is_empty_call(tmpl[3]) and is_data(tmpl[2], defined):
        cond, keep_when, out_tmpl = tmpl[1], 1, tmpl[2]
    elif not is_data(tmpl, defined):
        # fnspec 3: the survey's if/case/let family (ranks 9, 11, 12): a template tree of
        # if (both branches templates), case on a bound value, (empty), data and boolean values
        tree = True
    family = 'exist' if exist else ('once' if once else shape_wrap)
    shape = family + '-' + ('cond' if tree else 'filter' if cond is not None else 'data')
    pat_vars = vars_of(tuple(pats))
    builtins = [('match', 3)]
    if shape_wrap == 'collapse':
        builtins.append(('collapse', 1))
    if once:
        builtins.append(('once', 1))
    if exist:
        builtins.append(('==', 2))
        if negated:
            builtins.append(('not', 1))
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

    cond_c, tree_c, needs_bool = None, None, False
    if tree:
        tree_c, tmpls, needs_bool = compile_tree(tmpl, args, pat_vars, defined, builtins, operand)
        out_tmpl = tuple(tmpls)
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
    # the body with its match replaced by the hosted call (same wrapper: collapse, once, ==, not)
    def subst(x):
        if x is match_node:
            return inner_h
        if is_expr(x):
            return '(' + ' '.join(subst(c) for c in x) + ')'
        return show(x)
    hosted_body = subst(body) if (exist or once or shape_wrap == 'collapse') else inner_h
    hosted_def = '(= (%s-h %s) %s)' % (fname, hargs, hosted_body)
    # fnspec 3, item 3 (upstream/cetta 0004): the function through match-rows' row hook; the runtime
    # registers the hook under a unique name in place of CPK_ROWS_HOOK.  The row is the tuple of the
    # compared variables and the template atoms (data leaves, case patterns) as the call instantiates
    # them; the generated row function reads them by position under each row's bindings.
    rows_row = '(' + ' '.join(['$' + v for v in cvars] + [show(t) for t in (out_tmpl if tree else (out_tmpl,))]) + ')'
    rows_pat = show(pat) if head(pat) == ',' or len(pats) == 1 else '(, ' + ' '.join(show(p) for p in pats) + ')'
    rows_def = '(= (cpk-rows:%s %s) (match-rows %s %s %s (CPK_ROWS_HOOK %s) (%s %s)))' % (
        fname, hargs, space_token if space_token else show(space), rows_pat, rows_row, hargs, fname, hargs)
    digest = hashlib.sha256(equation.encode()).hexdigest()
    return dict(name=name, fname=fname, arity=len(args), shape=shape, space_arg=space_arg, space_token=space_token,
                parts=parts, equation=equation, builtins=builtins, cond_c=cond_c, n_cvars=len(cvars),
                n_consts=len(consts), hosted_def=hosted_def, digest=digest, source=path, text=text, meta=meta,
                n_patterns=len(pats), once=once, exist=exist, tree_c=tree_c, needs_bool=needs_bool, rows_def=rows_def)

def emit(s):
    cid = re.sub(r'[^A-Za-z0-9_]', '_', s['name'])
    shape_c = {'stream-data': 'FS_STREAM_DATA', 'collapse-data': 'FS_COLLAPSE_DATA',
               'collapse-filter': 'FS_COLLAPSE_FILTER', 'stream-filter': 'FS_STREAM_FILTER',
               'once-data': 'FS_ONCE_DATA', 'once-filter': 'FS_ONCE_FILTER',
               'exist-data': 'FS_EXIST_DATA', 'exist-filter': 'FS_EXIST_FILTER',
               'stream-cond': 'FS_STREAM_COND', 'collapse-cond': 'FS_COLLAPSE_COND',
               'once-cond': 'FS_ONCE_COND', 'exist-cond': 'FS_EXIST_COND'}[s['shape']]
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
    if s['tree_c']:
        out.append('/* fnspec 3: the template tree, compiled */\n' + s['tree_c'] + '\n')
        out.append('static int row_%s(FsCall *c, Bindings *row, Arena *a, Atom **answer) {\n'
                   '    return tree_top(c, row, a, answer);\n}\n\n' % cid)
    else:
        out.append('static int row_%s(FsCall *c, Bindings *row, Arena *a, Atom **answer) {\n' % cid)
    if s['tree_c']:
        pass
    elif s['cond_c']:
        out.append('    int k = cond_%s(c, row, a);\n    if (k <= 0) return k;\n' % cid)
    else:
        out.append('    /* data template: nothing to evaluate */\n')
    if not s['tree_c']:
        out.append('    *answer = FS_TEMPLATE();\n'
                   '    if (!*answer || !fs_normal_form(c, *answer)) return -1;\n'
                   '    return 1;\n}\n\n')
    b = ', '.join('{%s, %d}' % (cstr(n), k) for n, k in s['builtins'])
    out.append('const FsPlan fs_plan_%s = {\n' % cid)
    out.append('    .site = %s, .function = %s, .arity = %d,\n' % (cstr(s['name']), cstr(s['fname']), s['arity']))
    out.append('    .shape = %s, .space_arg = %d, .space_token = %s,\n'
               % (shape_c, s['space_arg'], cstr(s['space_token']) if s['space_token'] else 'NULL'))
    for k in ('parts', 'equation', 'hosted_def', 'rows_def', 'digest'):
        out.append('    .%s = %s,\n' % (k, cstr(s[k])))
    out.append('    .definitions = %s,\n' % cstr(defs))
    out.append('    .builtins = {%s}, .n_builtins = %d,\n' % (b, len(s['builtins'])))
    out.append('    .n_cvars = %d, .n_consts = %d, .n_patterns = %d,\n' % (s['n_cvars'], s['n_consts'], s['n_patterns']))
    out.append('    .once = %d, .exist = %d,\n' % (1 if s['once'] else 0, s['exist']))
    out.append('    .tree = %d, .needs_bool = %d,\n' % (1 if s['tree_c'] else 0, 1 if s['needs_bool'] else 0))
    out.append('    .row = row_%s,\n' % cid)
    for k in ('kb', 'queries', 'origin', 'setup', 'profile', 'load'):
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
