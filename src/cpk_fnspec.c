/* Runtime for generated whole-function plans: guards, residual, callables.
 * See cpk_fnspec.h for the guard order. */
#define _POSIX_C_SOURCE 200809L
#include "cpk_fnspec.h"
#include "grounded.h"
#include "symbol.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *fs_shape_name(FsShape s) {
    switch (s) {
    case FS_STREAM_DATA: return "stream-data";
    case FS_COLLAPSE_DATA: return "collapse-data";
    case FS_COLLAPSE_FILTER: return "collapse-filter";
    case FS_STREAM_FILTER: return "stream-filter";
    case FS_ONCE_DATA: return "once-data";
    case FS_ONCE_FILTER: return "once-filter";
    case FS_EXIST_DATA: return "exist-data";
    case FS_EXIST_FILTER: return "exist-filter";
    case FS_STREAM_COND: return "stream-cond";
    case FS_COLLAPSE_COND: return "collapse-cond";
    case FS_ONCE_COND: return "once-cond";
    case FS_EXIST_COND: return "exist-cond";
    }
    return "?";
}

const char *fs_profile(const FsPlan *p) {
    return p->profile && *p->profile ? p->profile : NULL;
}

const FsPlan *fs_find(const char *site) {
    for (const FsPlan *const *p = fs_plans; *p; p++)
        if (!strcmp((*p)->site, site)) return *p;
    return NULL;
}

static Space *space_of_native(const Atom *x) {
    return x && x->kind == ATOM_GROUNDED && x->ground.gkind == GV_SPACE ? x->ground.ptr : NULL;
}

/* the named space of a compiled body (the space the token resolved to): the one space
 * atom below its wrappers (collapse, once, ==, not); the compiler admits no other */
static const Atom *find_space(const Atom *x) {
    if (!x) return NULL;
    if (space_of_native(x)) return x;
    if (x->kind != ATOM_EXPR) return NULL;
    for (CettaExprLen i = 0; i < x->expr.len; i++) {
        const Atom *m = find_space(x->expr.elems[i]);
        if (m) return m;
    }
    return NULL;
}

/* ---- value kinds -------------------------------------------------------------- */

static bool is_number(const Atom *x) {
    return x->kind == ATOM_GROUNDED && (x->ground.gkind == GV_INT || x->ground.gkind == GV_FLOAT ||
                                        x->ground.gkind == GV_BIGINT || x->ground.gkind == GV_RATIONAL);
}

/* ground, and no number below the top (so that structural equality is CeTTa's ==) */
static bool plain_ground(const Atom *x, bool top) {
    switch (x->kind) {
    case ATOM_VAR: return false;
    case ATOM_SYMBOL: return true;
    case ATOM_GROUNDED:
        if (is_number(x)) return top;
        return x->ground.gkind == GV_STRING || x->ground.gkind == GV_BOOL;
    case ATOM_EXPR:
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!plain_ground(x->expr.elems[i], false)) return false;
        return true;
    }
    return false;
}

static bool nf_rec(FsRt *rt, const Atom *x) {
    switch (x->kind) {
    case ATOM_VAR: return false;
    case ATOM_SYMBOL:
        return !space_equations_may_match_known_head(rt->self, x->sym_id);
    case ATOM_GROUNDED:
        return x->ground.gkind == GV_INT || x->ground.gkind == GV_FLOAT || x->ground.gkind == GV_STRING ||
               x->ground.gkind == GV_BOOL;
    case ATOM_EXPR: {
        if (x->expr.len == 0) return true;
        const Atom *h = x->expr.elems[0];
        if (h->kind == ATOM_SYMBOL) {
            if (is_grounded_op(h->sym_id) || symbol_id_is_builtin(h->sym_id)) return false;
        } else if (h->kind == ATOM_VAR) {
            return false;
        } else if (h->kind == ATOM_GROUNDED) {
            /* a number, string or bool at the head is data (no operation to run);
             * any other grounded head may be executable */
            GroundedKind g = h->ground.gkind;
            if (g != GV_INT && g != GV_FLOAT && g != GV_STRING && g != GV_BOOL) return false;
        } else if (space_equations_may_match_unnamed_head(rt->self)) {
            return false;                                   /* an expression head */
        }
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!nf_rec(rt, x->expr.elems[i])) return false;
        return true;
    }
    }
    return false;
}

bool fs_normal_form(FsCall *c, Atom *x) {
    return x && nf_rec(c->rt, x);
}

int fs_compare(FsCall *c, FsCmp op, Atom *x, Atom *y) {
    if (!x || !y) return -1;
    /* CeTTa evaluates both operands first: they must already be values */
    if (!nf_rec(c->rt, x) || !nf_rec(c->rt, y)) return -1;
    bool nx = is_number(x), ny = is_number(y);
    if (op == FS_EQ || op == FS_ALPHA) {
        if (nx || ny) {
            if (!nx || !ny || x->ground.gkind != y->ground.gkind) return -1;   /* mixed kinds: CeTTa's */
            if (x->ground.gkind == GV_INT) return x->ground.ival == y->ground.ival;
            if (x->ground.gkind == GV_FLOAT) {
                if (isnan(x->ground.fval) || isnan(y->ground.fval)) return -1;
                return x->ground.fval == y->ground.fval;
            }
            return -1;                                                        /* bigint, rational */
        }
        if (!plain_ground(x, true) || !plain_ground(y, true)) return -1;
        return atom_eq(x, y) ? 1 : 0;
    }
    if (!nx || !ny || x->ground.gkind != y->ground.gkind) return -1;
    if (x->ground.gkind == GV_INT) {
        int64_t a = x->ground.ival, b = y->ground.ival;
        switch (op) {
        case FS_LT: return a < b;
        case FS_GT: return a > b;
        case FS_LE: return a <= b;
        case FS_GE: return a >= b;
        default: return -1;
        }
    }
    if (x->ground.gkind == GV_FLOAT) {
        double a = x->ground.fval, b = y->ground.fval;
        if (isnan(a) || isnan(b)) return -1;
        switch (op) {
        case FS_LT: return a < b;
        case FS_GT: return a > b;
        case FS_LE: return a <= b;
        case FS_GE: return a >= b;
        default: return -1;
        }
    }
    return -1;
}

/* ---- embedding helpers ---------------------------------------------------------- */

static cetta_atom_t *parse_text(cetta_engine_t *e, const char *t) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_parse(e, t, strlen(t), &a) != CETTA_OK) return NULL;
    return a;
}


/* an atom as a MeTTa file would read it: tokens bound with bind! (named spaces)
 * resolved, which cetta_embed_parse does not do: !(quote TEXT), then its argument */
static cetta_atom_t *read_resolved(cetta_engine_t *e, const char *t) {
    size_t n = strlen(t) + 16;
    char *src = malloc(n);
    snprintf(src, n, "!(quote %s)", t);
    cetta_result_t *r = NULL;
    cetta_atom_t *out = NULL;
    if (cetta_embed_run(e, src, strlen(src), NULL, &r) == CETTA_OK && cetta_embed_result_status(r) == CETTA_OK &&
        cetta_embed_result_count(r) == 1) {
        cetta_atom_t *q = NULL;
        cetta_embed_result_get(r, 0, &q);
        if (cetta_embed_atom_kind(q) == CETTA_ATOM_EXPRESSION && cetta_embed_atom_arity(q) == 2)
            cetta_embed_atom_child(q, 1, &out);
        cetta_embed_atom_release(q);
    }
    cetta_embed_result_release(r);
    free(src);
    return out;
}

/* text of an atom, variables renamed by first occurrence */
typedef struct { char *buf; size_t len, cap; VarId ids[256]; unsigned n; bool ok; } Canon;
static void canon_put(Canon *k, const char *s) {
    size_t l = strlen(s);
    if (k->len + l + 1 > k->cap) {
        size_t cap = (k->cap ? k->cap * 2 : 256) + l;
        char *t = realloc(k->buf, cap);
        if (!t) { k->ok = false; return; }
        k->buf = t; k->cap = cap;
    }
    memcpy(k->buf + k->len, s, l + 1);
    k->len += l;
}
static void canon_rec(Canon *k, const cetta_atom_t *a) {
    if (!k->ok) return;
    const Atom *n = cetta_embed_native_atom(a);
    if (n && n->kind == ATOM_VAR) {
        unsigned i = 0;
        while (i < k->n && !var_id_eq(k->ids[i], n->var_id)) i++;
        if (i == k->n) { if (k->n == 256) { k->ok = false; return; } k->ids[k->n++] = n->var_id; }
        char v[32]; snprintf(v, sizeof v, "$_%u", i); canon_put(k, v);
        return;
    }
    if (cetta_embed_atom_kind(a) == CETTA_ATOM_EXPRESSION) {
        size_t m = cetta_embed_atom_arity(a);
        canon_put(k, "(");
        for (size_t i = 0; i < m; i++) {
            cetta_atom_t *ch = NULL;
            if (cetta_embed_atom_child(a, i, &ch) != CETTA_OK) { k->ok = false; return; }
            if (i) canon_put(k, " ");
            canon_rec(k, ch);
            cetta_embed_atom_release(ch);
        }
        canon_put(k, ")");
        return;
    }
    size_t need = 0;
    cetta_embed_atom_format(a, NULL, 0, &need);
    char *s = malloc(need + 1);
    if (!s || cetta_embed_atom_format(a, s, need + 1, &need) != CETTA_OK) { free(s); k->ok = false; return; }
    canon_put(k, s);
    free(s);
}
static char *canon_text(const cetta_atom_t *a) {
    Canon k = {.ok = true};
    canon_put(&k, "");
    canon_rec(&k, a);
    if (!k.ok) { free(k.buf); return NULL; }
    return k.buf;
}

/* canonical text of every result of a query, in result order, as one string */
static char *eval_canon(cetta_engine_t *e, const cetta_atom_t *q, int *count) {
    cetta_result_t *r = NULL;
    *count = -1;
    if (cetta_embed_eval(e, q, NULL, &r) != CETTA_OK || cetta_embed_result_status(r) != CETTA_OK) {
        cetta_embed_result_release(r);
        return NULL;
    }
    Canon k = {.ok = true};
    canon_put(&k, "");
    size_t n = cetta_embed_result_count(r);
    for (size_t i = 0; i < n; i++) {
        cetta_atom_t *h = NULL;
        cetta_embed_result_get(r, i, &h);
        char *t = canon_text(h);
        cetta_embed_atom_release(h);
        if (!t) { k.ok = false; break; }
        if (i) canon_put(&k, " | ");
        canon_put(&k, t);
        free(t);
    }
    cetta_embed_result_release(r);
    if (!k.ok) { free(k.buf); return NULL; }
    *count = (int)n;
    return k.buf;
}

/* alpha-equality of two native atoms (variables by a bijection) */
typedef struct { VarId a[256], b[256]; unsigned n; } VarMap;
static bool alpha_rec(const Atom *x, const Atom *y, VarMap *m) {
    if (x->kind != y->kind) return false;
    if (x->kind == ATOM_VAR) {
        for (unsigned i = 0; i < m->n; i++) {
            bool ea = var_id_eq(m->a[i], x->var_id), eb = var_id_eq(m->b[i], y->var_id);
            if (ea || eb) return ea && eb;
        }
        if (m->n == 256) return false;
        m->a[m->n] = x->var_id; m->b[m->n] = y->var_id; m->n++;
        return true;
    }
    if (x->kind == ATOM_EXPR) {
        if (x->expr.len != y->expr.len) return false;
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!alpha_rec(x->expr.elems[i], y->expr.elems[i], m)) return false;
        return true;
    }
    return atom_eq((Atom *)x, (Atom *)y);
}

/* ---- the guard ---------------------------------------------------------------- */

static void revalidate(FsRt *rt) {
    rt->st.revalidations++;
    rt->valid = false;
    rt->tok = space_program_token(rt->self);
    rt->tok_valid = true;
    if (rt->once_unavailable) { rt->invalid_reason = "once is not CeTTa's first-answer builtin in this engine's profile"; return; }
    if (rt->eq_mismatch) { rt->invalid_reason = "the engine's existence-test answers (== / not probe) differ from he's True/False"; return; }
    if (space_equations_may_match_unnamed_head(rt->self)) { rt->invalid_reason = "catch-all equation in &self"; return; }
    /* the definition: exactly one equation of f, alpha-equal to the compiled one */
    cetta_result_t *r = NULL;
    if (cetta_embed_eval(rt->engine, rt->def_query_h, NULL, &r) != CETTA_OK || cetta_embed_result_status(r) != CETTA_OK) {
        cetta_embed_result_release(r);
        rt->invalid_reason = "definition query failed";
        return;
    }
    size_t n = cetta_embed_result_count(r);
    bool same = false;
    if (n == 1) {
        cetta_atom_t *h = NULL;
        cetta_embed_result_get(r, 0, &h);
        const Atom *q = cetta_embed_native_atom(h);        /* (quote (= LHS BODY)) */
        if (q && q->kind == ATOM_EXPR && q->expr.len == 2) {
            VarMap m = {.n = 0};
            same = alpha_rec(q->expr.elems[1], rt->equation, &m);
        }
        cetta_embed_atom_release(h);
    }
    cetta_embed_result_release(r);
    if (n != 1) { rt->invalid_reason = n ? "more than one equation of the function" : "no equation of the function"; return; }
    if (!same) { rt->invalid_reason = "the function's equation differs from the compiled one"; return; }
    int cnt = 0;
    char *d = eval_canon(rt->engine, rt->decl_query_h, &cnt);
    bool decl_ok = d && cnt == 0;
    free(d);
    if (!decl_ok) { rt->invalid_reason = "a type declaration of the function"; return; }
    /* builtin meanings: equations and declarations as in a fresh engine */
    for (unsigned i = 0; i < 2 * rt->plan->n_builtins; i++) {
        char *t = eval_canon(rt->engine, rt->builtin_query_h[i], &cnt);
        bool ok = t && rt->baseline[i] && !strcmp(t, rt->baseline[i]);
        free(t);
        if (!ok) { rt->invalid_reason = "a builtin the body uses has an equation or declaration a fresh engine does not"; return; }
    }
    rt->valid = true;
    rt->invalid_reason = NULL;
}

/* ---- calls ---------------------------------------------------------------------- */

typedef struct {
    FsRt *rt;
    bool hosted;
    cetta_atom_t *args[16];
    size_t nargs;
    cetta_atom_t **answers;
    size_t n, cap, pos;
    bool computed, failed;
} Cursor;

static int push(Cursor *c, cetta_atom_t *h) {
    if (c->n == c->cap) {
        size_t cap = c->cap ? c->cap * 2 : 16;
        cetta_atom_t **t = realloc(c->answers, cap * sizeof(*t));
        if (!t) return -1;
        c->answers = t; c->cap = cap;
    }
    c->answers[c->n++] = h;
    return 0;
}
static void drop_answers(Cursor *c) {
    for (size_t i = 0; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
    c->n = 0;
}

static bool residual(Cursor *c) {
    FsRt *rt = c->rt;
    rt->st.residual_calls++;
    cetta_atom_t *items[17];
    if (cetta_embed_atom_symbol(rt->engine, rt->plan->function, &items[0]) != CETTA_OK) return false;
    for (size_t i = 0; i < c->nargs; i++) items[i + 1] = c->args[i];
    cetta_atom_t *x = NULL;
    bool ok = cetta_embed_atom_expression(rt->engine, items, c->nargs + 1, &x) == CETTA_OK;
    cetta_embed_atom_release(items[0]);
    cetta_result_t *r = NULL;
    ok = ok && cetta_embed_eval(rt->engine, x, NULL, &r) == CETTA_OK && cetta_embed_result_status(r) == CETTA_OK;
    for (size_t i = 0; ok && i < cetta_embed_result_count(r); i++) {
        cetta_atom_t *h = NULL;
        ok = cetta_embed_result_get(r, i, &h) == CETTA_OK && push(c, h) == 0;
    }
    cetta_embed_result_release(r);
    cetta_embed_atom_release(x);
    return ok;
}

static Space *call_space(FsRt *rt, Cursor *c) {
    if (rt->plan->space_arg >= 0) return cetta_embed_native_space(c->args[rt->plan->space_arg]);
    if (c->hosted) return cetta_embed_native_space(c->args[0]);
    return rt->named_space ? space_of_native(rt->named_space) : NULL;
}

/* returns 1 done, 0 abort (residual) */
static int compute(Cursor *c) {
    FsRt *rt = c->rt;
    const FsPlan *p = rt->plan;
    size_t off = (c->hosted && p->space_arg < 0) ? 1 : 0;   /* hosted callable of a named-space plan: space first */
    Space *s = call_space(rt, c);
    if (!s) return 0;
    Atom *nargs[16];
    for (unsigned i = 0; i < p->arity; i++) {
        nargs[i] = (Atom *)cetta_embed_native_atom(c->args[i + off]);
        if (!nargs[i]) return 0;
        if ((int)i != p->space_arg && !c->hosted) {
            FsCall fc = {rt, nargs};
            if (!fs_normal_form(&fc, nargs[i]) || !plain_ground(nargs[i], true)) { rt->st.arg_rejects++; return 0; }
        }
    }
    FsCall fc = {rt, nargs};
    Arena a;
    arena_init_detached(&a);
    Bindings sb;
    bindings_init(&sb);
    bool ok = true;
    for (unsigned i = 0; ok && i < p->arity; i++)
        if ((int)i != p->space_arg) ok = bindings_add_var(&sb, rt->lhs_args[i], nargs[i]);
    Atom **raw = NULL;
    size_t nraw = 0, rawcap = 0, filtered = 0;
    /* collapse(once) and existence tests need only the first surviving row (matcher order) */
    bool cut = !c->hosted && (p->once || p->exist);
    if (ok) {
        BindingSet rows;
        space_query_conjunction(s, &a, rt->pats, (CettaExprLen)p->n_patterns, &sb, &rows);
        for (CettaCount i = 0; ok && i < rows.len; i++) {
            Atom *ans = NULL;
            int k;
            if (c->hosted) {
                ans = bindings_apply(&rows.items[i], &a, rt->hvars);
                k = ans ? 1 : -1;
            } else {
                k = p->row(&fc, &rows.items[i], &a, &ans);
            }
            if (k < 0) { ok = false; break; }
            if (k == 0) { filtered++; continue; }
            if (k == 2) continue;                       /* a tree's case with no matching branch */
            if (nraw == rawcap) {
                rawcap = rawcap ? rawcap * 2 : 16;
                Atom **t = realloc(raw, rawcap * sizeof(*t));
                if (!t) { ok = false; break; }
                raw = t;
            }
            raw[nraw++] = ans;
            if (cut) break;
        }
        binding_set_free(&rows);
    }
    bool collapse = !c->hosted && (p->shape == FS_COLLAPSE_DATA || p->shape == FS_COLLAPSE_FILTER ||
                                   p->shape == FS_ONCE_DATA || p->shape == FS_ONCE_FILTER ||
                                   p->shape == FS_COLLAPSE_COND || p->shape == FS_ONCE_COND);
    if (ok && !c->hosted && p->exist) {
        /* (not (== W ())) is True when a row survives, (== W ()) when none does */
        bool val = p->exist == 1 ? nraw > 0 : nraw == 0;
        cetta_atom_t *h = val ? rt->true_h : rt->false_h;
        cetta_embed_atom_retain(h);
        ok = push(c, h) == 0;
        if (!ok) cetta_embed_atom_release(h);
        collapse = false;
        nraw = 0;
    }
    if (ok && !c->hosted && (p->shape == FS_STREAM_FILTER || p->shape == FS_STREAM_COND) && nraw == 0 && filtered > 0) {
        /* CeTTa's result for a stream whose every row takes the (empty) branch:
         * one Empty per row (docs/NEXT_STEP_PROBES.md, bag tests d5/d6) */
        Atom *empty = atom_symbol(&a, "Empty");
        free(raw);
        raw = malloc(filtered * sizeof(*raw));
        ok = raw != NULL;
        for (size_t i = 0; ok && i < filtered; i++) raw[nraw++] = empty;
    }
    if (ok && collapse) {
        cetta_atom_t *h = NULL;
        ok = cetta_embed_native_atom_copy(rt->engine, atom_expr(&a, raw, (CettaExprLen)nraw), &h) == CETTA_OK && push(c, h) == 0;
    } else if (ok && nraw) {
        cetta_atom_t *bundle = NULL;
        ok = cetta_embed_native_atom_copy(rt->engine, atom_expr(&a, raw, (CettaExprLen)nraw), &bundle) == CETTA_OK;
        for (size_t i = 0; ok && i < nraw; i++) {
            cetta_atom_t *h = NULL;
            ok = cetta_embed_atom_child(bundle, i, &h) == CETTA_OK && push(c, h) == 0;
        }
        cetta_embed_atom_release(bundle);
    }
    free(raw);
    bindings_free(&sb);
    arena_free(&a);
    if (!ok) { drop_answers(c); return 0; }
    return 1;
}

static void run_call(Cursor *c) {
    FsRt *rt = c->rt;
    if (c->hosted) {
        rt->st.hosted_calls++;
        if (!compute(c)) c->failed = true;
        return;
    }
    rt->st.calls++;
    bool spec = false;
    if (rt->force_invalid) rt->st.forced++;
    else {
        if (!rt->tok_valid || !space_program_token_is_current(rt->tok)) revalidate(rt);
        if (!rt->valid) rt->st.invalid++;
        else {
            uint64_t rejects = rt->st.arg_rejects;
            spec = compute(c);
            if (!spec && rt->st.arg_rejects == rejects) rt->st.row_aborts++;
        }
    }
    if (spec) { rt->st.spec_calls++; return; }
    if (!residual(c)) { drop_answers(c); c->failed = true; }
}

static cetta_status_t cb_open(FsRt *rt, bool hosted, cetta_atom_t *const *args, size_t arity, void **out) {
    Cursor *c = calloc(1, sizeof(*c));
    if (!c || arity > 16) { free(c); return CETTA_OUT_OF_MEMORY; }
    c->rt = rt; c->hosted = hosted; c->nargs = arity;
    for (size_t i = 0; i < arity; i++) c->args[i] = args[i];
    *out = c;
    return CETTA_OK;
}
static cetta_status_t spec_open(void *ctx, cetta_engine_t *e, cetta_atom_t *const *args, size_t arity, void **out) {
    (void)e; return cb_open(ctx, false, args, arity, out);
}
static cetta_status_t hosted_open(void *ctx, cetta_engine_t *e, cetta_atom_t *const *args, size_t arity, void **out) {
    (void)e; return cb_open(ctx, true, args, arity, out);
}
static cetta_status_t cb_next(void *cursor, cetta_callback_state_t *state, cetta_atom_t **out) {
    Cursor *c = cursor;
    *out = NULL;
    if (!c->computed) { run_call(c); c->computed = true; }
    if (c->failed) { *state = CETTA_CALLBACK_COMPLETE; return CETTA_ENGINE_ERROR; }
    if (c->pos < c->n) { *out = c->answers[c->pos++]; *state = CETTA_CALLBACK_VALUE; return CETTA_OK; }
    *state = CETTA_CALLBACK_COMPLETE;
    return CETTA_OK;
}
static void cb_close(void *cursor, cetta_callback_close_mode_t mode) {
    (void)mode;
    Cursor *c = cursor;
    for (size_t i = c->pos; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
    free(c->answers);
    free(c);
}
static void cb_nop(void *ctx) { (void)ctx; }

/* ---- open ------------------------------------------------------------------------ */

static char *fmt_s(const char *f, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static char *fmt_s(const char *f, ...) {
    va_list ap; va_start(ap, f);
    int n = vsnprintf(NULL, 0, f, ap);
    va_end(ap);
    char *s = malloc((size_t)n + 1);
    va_start(ap, f); vsnprintf(s, (size_t)n + 1, f, ap); va_end(ap);
    return s;
}

static char *vars_text(const char *prefix, unsigned n) {
    char *s = calloc(1, 1);
    for (unsigned i = 0; i < n; i++) {
        char *t = fmt_s("%s $%s%u", s, prefix, i);
        free(s); s = t;
    }
    return s;
}

static void builtin_queries(cetta_engine_t *e, const FsPlan *p, cetta_atom_t **out) {
    for (unsigned i = 0; i < p->n_builtins; i++) {
        char *v = vars_text("x", p->builtins[i].arity);
        const char *nm = p->builtins[i].name;
        char *q1 = p->builtins[i].arity ? fmt_s("(match &self (= (%s%s) $r) (quote (= (%s%s) $r)))", nm, v, nm, v)
                                        : fmt_s("(match &self (= (%s) $r) (quote (= (%s) $r)))", nm, nm);
        char *q2 = fmt_s("(match &self (: %s $t) (quote $t))", nm);
        out[2 * i] = parse_text(e, q1);
        out[2 * i + 1] = parse_text(e, q2);
        free(v); free(q1); free(q2);
    }
}

FsRt *fs_open(cetta_engine_t *e, const FsPlan *p, bool force_invalid, const char **error) {
    FsRt *rt = calloc(1, sizeof(*rt));
    rt->plan = p; rt->engine = e; rt->force_invalid = force_invalid;
    *error = NULL;
    rt->parts_h = p->space_token ? read_resolved(e, p->parts) : parse_text(e, p->parts);
    rt->equation_h = p->space_token ? read_resolved(e, p->equation) : parse_text(e, p->equation);
    if (!rt->parts_h || !rt->equation_h) { *error = "parse plan texts"; return NULL; }
    rt->equation = (Atom *)cetta_embed_native_atom(rt->equation_h);
    const Atom *all = cetta_embed_native_atom(rt->parts_h);
    if (!all || all->kind != ATOM_EXPR || all->expr.len != 8) { *error = "plan parts shape"; return NULL; }
    const Atom *lhs = all->expr.elems[1], *pats = all->expr.elems[2];
    rt->tmpl = all->expr.elems[3];
    const Atom *cv = all->expr.elems[4], *ks = all->expr.elems[5];
    rt->hvars = all->expr.elems[6];
    rt->lhs_args = calloc(p->arity + 1, sizeof(Atom *));
    for (unsigned i = 0; i < p->arity; i++) rt->lhs_args[i] = lhs->expr.elems[i + 1];
    rt->pats = calloc(pats->expr.len + 1, sizeof(Atom *));
    for (CettaExprLen i = 0; i < pats->expr.len; i++) rt->pats[i] = pats->expr.elems[i];
    rt->cvars = calloc(cv->expr.len + 1, sizeof(Atom *));
    for (CettaExprLen i = 0; i < cv->expr.len; i++) rt->cvars[i] = cv->expr.elems[i];
    rt->consts = calloc(ks->expr.len + 1, sizeof(Atom *));
    for (CettaExprLen i = 0; i < ks->expr.len; i++) rt->consts[i] = ks->expr.elems[i];
    if (p->tree) {
        if (rt->tmpl->kind != ATOM_EXPR) { *error = "tree templates"; return NULL; }
        rt->tmpls = calloc(rt->tmpl->expr.len + 1, sizeof(Atom *));
        for (CettaExprLen i = 0; i < rt->tmpl->expr.len; i++) rt->tmpls[i] = rt->tmpl->expr.elems[i];
    }
    cetta_space_t *selfs = NULL;
    if (cetta_embed_engine_space(e, &selfs) != CETTA_OK) { *error = "engine space"; return NULL; }
    cetta_atom_t *self_atom = NULL;
    cetta_embed_space_atom(selfs, &self_atom);
    rt->self = cetta_embed_native_space(self_atom);
    cetta_embed_atom_release(self_atom);
    cetta_embed_space_release(selfs);
    if (!rt->self) { *error = "no native &self"; return NULL; }
    if (p->space_token) {
        /* the token's space atom as the compiled body's match holds it */
        rt->named_space = (Atom *)find_space(rt->equation->expr.elems[2]);
        if (!rt->named_space) { *error = "named space token is not a space"; return NULL; }
        if (!space_of_native(rt->named_space)) { *error = "named space token is not a space"; return NULL; }
    }
    /* guard queries */
    char *v = vars_text("a", p->arity);
    char *dq = fmt_s("(match &self (= (%s%s) $b) (quote (= (%s%s) $b)))", p->function, v, p->function, v);
    char *cq = fmt_s("(match &self (: %s $t) (quote $t))", p->function);
    rt->def_query_h = parse_text(e, dq);
    rt->decl_query_h = parse_text(e, cq);
    free(v); free(dq); free(cq);
    rt->builtin_query_h = calloc(2 * p->n_builtins + 1, sizeof(cetta_atom_t *));
    builtin_queries(e, p, rt->builtin_query_h);
    /* baseline: the builtins' equations and declarations in a fresh engine */
    rt->baseline = calloc(2 * p->n_builtins + 1, sizeof(char *));
    cetta_engine_config_t cfg = {.size = sizeof cfg, .abi_version = CETTA_EMBED_ABI_VERSION,
                                 .language = "he", .profile = fs_profile(p), .resource_root = CPK_CETTA_ROOT};
    cetta_engine_t *fresh = NULL;
    if (cetta_embed_engine_new(&cfg, &fresh) != CETTA_OK) { *error = "baseline engine"; return NULL; }
    /* whether once is the first-answer builtin in this engine (a known-answer probe in the
     * engine itself: its profile decides; a probe that fails for any reason, e.g. a shadowed
     * superpose at open time, leaves the plan INVALID, never wrongly valid); and the values
     * == returns, read in a fresh engine */
    if (p->once) {
        cetta_atom_t *q = parse_text(e, "(collapse (once (superpose (cpk-once-a cpk-once-b))))");
        int cnt = 0;
        char *t = q ? eval_canon(e, q, &cnt) : NULL;
        rt->once_unavailable = !(t && cnt == 1 && !strcmp(t, "(cpk-once-a)"));
        free(t);
        cetta_embed_atom_release(q);
    }
    if (p->exist || p->needs_bool) {
        static const char *probe[2] = {"(== () ())", "(== () (cpk-exist-x))"};
        cetta_atom_t **dst[2] = {&rt->true_h, &rt->false_h};
        for (int i = 0; i < 2; i++) {
            cetta_atom_t *q = parse_text(fresh, probe[i]), *h = NULL;
            cetta_result_t *res = NULL;
            if (q && cetta_embed_eval(fresh, q, NULL, &res) == CETTA_OK && cetta_embed_result_status(res) == CETTA_OK &&
                cetta_embed_result_count(res) == 1 && cetta_embed_result_get(res, 0, &h) == CETTA_OK) {
                if (cetta_embed_native_atom_copy(e, cetta_embed_native_atom(h), dst[i]) != CETTA_OK) *dst[i] = NULL;
                cetta_embed_atom_release(h);
            }
            cetta_embed_result_release(res);
            cetta_embed_atom_release(q);
        }
        if (!rt->true_h || !rt->false_h) { cetta_embed_engine_release(fresh); *error = "boolean values of =="; return NULL; }
        /* fnspec 3 (live == probe, like once's): the compiled code answers he's True/False above;
         * the live engine must give exactly those atoms for the test the body makes, with an
         * empty and a non-empty W, in both operand orders, under not when the body negates.
         * Any other answer (e.g. a PeTTa-language engine's true/false), a count other than one,
         * or a failed probe leaves the plan INVALID for good: every call takes the residual. */
        static const char *wv[2] = {"()", "(cpk-exist-x)"};
        char *tt = canon_text(rt->true_h), *ft = canon_text(rt->false_h);
        for (int w = 0; p->exist && w < 2 && !rt->eq_mismatch; w++)
            for (int ord = 0; ord < 2 && !rt->eq_mismatch; ord++) {
                char q[128];
                const char *x = ord ? "()" : wv[w], *y = ord ? wv[w] : "()";
                if (p->exist == 1) snprintf(q, sizeof q, "(not (== %s %s))", x, y);
                else snprintf(q, sizeof q, "(== %s %s)", x, y);
                /* W empty: exist 1 answers False, exist 2 True; W non-empty the other one */
                bool want_true = (w == 1) == (p->exist == 1);
                cetta_atom_t *qa = parse_text(e, q);
                int cnt = 0;
                char *t = qa ? eval_canon(e, qa, &cnt) : NULL;
                const char *want = want_true ? tt : ft;
                if (!t || cnt != 1 || !want || strcmp(t, want)) rt->eq_mismatch = true;
                free(t);
                cetta_embed_atom_release(qa);
            }
        /* a tree that answers booleans: the live engine's comparison and connective values */
        static const char *bq[5][2] = {{"(== cpk-a cpk-a)", "T"}, {"(== cpk-a cpk-b)", "F"}, {"(not (== cpk-a cpk-b))", "T"},
                                       {"(and True False)", "F"}, {"(or False True)", "T"}};
        for (int i = 0; p->needs_bool && i < 5 && !rt->eq_mismatch; i++) {
            cetta_atom_t *qa = parse_text(e, bq[i][0]);
            int cnt = 0;
            char *t = qa ? eval_canon(e, qa, &cnt) : NULL;
            const char *want = bq[i][1][0] == 'T' ? tt : ft;
            if (!t || cnt != 1 || !want || strcmp(t, want)) rt->eq_mismatch = true;
            free(t);
            cetta_embed_atom_release(qa);
        }
        free(tt); free(ft);
        rt->true_a = (Atom *)cetta_embed_native_atom(rt->true_h);
        rt->false_a = (Atom *)cetta_embed_native_atom(rt->false_h);
    }
    cetta_atom_t **bq = calloc(2 * p->n_builtins + 1, sizeof(cetta_atom_t *));
    builtin_queries(fresh, p, bq);
    for (unsigned i = 0; i < 2 * p->n_builtins; i++) {
        int cnt = 0;
        rt->baseline[i] = eval_canon(fresh, bq[i], &cnt);
        cetta_embed_atom_release(bq[i]);
    }
    free(bq);
    cetta_embed_engine_release(fresh);
    char name[256];
    cetta_callback_descriptor_t d = {
        .size = sizeof d, .arity = p->arity, .kind = CETTA_CALLBACK_STREAM, .context = rt,
        .retain = cb_nop, .release = cb_nop, .open = spec_open, .next = cb_next, .close = cb_close,
    };
    snprintf(name, sizeof name, "cpk-fnspec:%s", p->function);
    if (cetta_embed_register_callback(e, name, &d, &rt->callable) != CETTA_OK) { *error = "register cpk-fnspec"; return NULL; }
    d.open = hosted_open;
    d.arity = p->arity + (p->space_arg < 0 ? 1 : 0);
    snprintf(name, sizeof name, "cpk-hosted:%s", p->function);
    if (cetta_embed_register_callback(e, name, &d, &rt->hosted) != CETTA_OK) { *error = "register cpk-hosted"; return NULL; }
    return rt;
}

void fs_close(FsRt *rt) {
    if (!rt) return;
    cetta_embed_atom_release(rt->callable);
    cetta_embed_atom_release(rt->hosted);
    cetta_embed_atom_release(rt->parts_h);
    cetta_embed_atom_release(rt->equation_h);
    if (rt->true_h) cetta_embed_atom_release(rt->true_h);
    if (rt->false_h) cetta_embed_atom_release(rt->false_h);
    cetta_embed_atom_release(rt->def_query_h);
    cetta_embed_atom_release(rt->decl_query_h);
    for (unsigned i = 0; i < 2 * rt->plan->n_builtins; i++) {
        cetta_embed_atom_release(rt->builtin_query_h[i]);
        free(rt->baseline[i]);
    }
    free(rt->builtin_query_h); free(rt->baseline);
    free(rt->lhs_args); free(rt->pats); free(rt->cvars); free(rt->consts); free(rt->tmpls);
    free(rt);
}
