/* cettapack S1 (t-2026-11-24-cettapack-s1-hot-sites): compiled match sites inside CeTTa's PeTTa runs.
 *
 * Built into CeTTa as src/cetta_s1_sites.c (bench/s1/build_s1_cetta.sh copies this file and
 * src/cpk_s1_sites_text.h over the stub of upstream/cetta-s1/0007), with `make CETTA_S1_SITES=1`.  The
 * interface is src/cetta_s1_sites.h of 0007: when PeTTa's search machine starts a match on a space, it offers
 * the match (reference, pattern, template as written, plus a way to apply the current bindings and to resolve
 * spaces) to cetta_s1_sites_try; a site that recognises it and whose guards hold returns the match's answers,
 * otherwise it declines having changed nothing and the machine runs the match itself (the residual).
 *
 * The guarantees are fnspec's (docs/FNSPEC.md), carried into the machine:
 *   - definition, per &self program revision: &self holds exactly one equation of the enclosing function,
 *     alpha-equal to the one the site was compiled from (the verbatim text of PeTTaChainer b0e24f9b,
 *     src/cpk_s1_sites_text.h); no catch-all equation; no equation in &self for any builtin the match's
 *     template uses.  Revalidated when space_program_token(&self) moves (equation/declaration revision).
 *   - recognition, per call: the match is the compiled one up to variable renaming (row variables must be
 *     unbound variables, input variables may be bound or unbound but consistent).
 *   - inputs, per call: the values in the pattern that the match does not bind are ground.
 *   - values, per row: an operand the template evaluates must already be a value (normal form, as fnspec's
 *     nf check, variables allowed since PeTTa leaves an unbound variable as it is); otherwise the whole call
 *     declines before anything is answered.
 *   - order, per call: the bag of answers must not depend on the matcher's row order (at most one distinct
 *     answer value), since the compiled query (space_query_conjunction) and the machine's match may enumerate
 *     rows in different orders and the site sits under once; otherwise the call declines.
 *   - $CETTA_S1_SITES=invalid forces every guard invalid (every call takes the residual; tests).
 * Counters per site are written at exit to $CETTA_S1_SITES_STATS (one JSON object), when set.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cetta_s1_sites.h"
#if CETTA_S1_SITES
#include "atom.h"
#include "grounded.h"
#include "match.h"
#include "parser.h"
#include "petta_semantics.h"
#include "space.h"
#include "symbol.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpk_s1_sites_text.h"

#define S1_MAX_VARS 32

typedef struct S1Site S1Site;

/* A compiled row: the values of the site's variables for one row (inputs and row variables, by the site's
 * variable index), the answer it gives (NULL: the row is filtered out).  Returns 1 answered, 0 filtered,
 * -1 decline the call (a value outside what the code computes as CeTTa does). */
typedef int (*S1RowFn)(const S1Site *site, Space *self, Arena *a, Atom **vals, Atom **answer);

struct S1Site {
    const char *name;
    const char *function;        /* the enclosing function's head */
    const char *equation_text;   /* verbatim from PeTTaChainer */
    const char *space;           /* the match's space reference */
    const char *row_vars[S1_MAX_VARS];    /* variables the match binds (unbound at the call) */
    const char *input_vars[S1_MAX_VARS];  /* variables bound before the match */
    const char *builtins[16];    /* builtins the template evaluates (their equations are guarded) */
    S1RowFn row;
    /* runtime */
    bool parsed, parse_failed;
    Atom *equation, *lhs_pattern_query; /* (= (f $a1..$an) $body) for the definition query */
    Atom *pattern, *template_atom;      /* the match's pattern and template, inside `equation` */
    VarId var_ids[S1_MAX_VARS];          /* site variable index -> VarId in `equation` */
    const char *var_names[S1_MAX_VARS];
    bool var_is_row[S1_MAX_VARS];
    unsigned nvars;
    SymbolId space_sym;
    SpaceProgramToken tok;
    bool tok_valid, valid;
    const char *invalid_reason;
    struct {
        uint64_t offered, recognised, specialised, answers, rows;
        uint64_t revalidations, res_invalid, res_forced, res_inputs, res_rowvar, res_value, res_order,
            res_space, res_query;
    } st;
};

static Arena g_s1_arena;
static bool g_s1_arena_init;
static int g_s1_mode = -1; /* 0 off, 1 on, 2 forced invalid */
static int g_s1_trace = 0;  /* $CETTA_S1_SITES_TRACE: one stderr line per call a site recognises */
#define S1_TRACE(s, what)                                                                                   \
    do {                                                                                                     \
        if (g_s1_trace)                                                                                      \
            fprintf(stderr, "s1-site %s call %" PRIu64 ": %s\n", (s)->name, (s)->st.recognised, (what));      \
    } while (0)

/* ---- the sites ----------------------------------------------------------------------------------------- */

/* forward-register-base-rate! (PeTTaChainer b0e24f9b forward_chainer.metta:385), the match under
 * (collapse (once ...)) that S0 ranked first in w3 (5.3% of self time):
 *   (match &forward_base_rate_interest
 *      (forward-base-rate-interest-row $trigger $stored-id $kbid $stored-pattern $stored-extractor $stored-init
 *                                      $fun $finish $tv-mode)
 *      (if (and (=alpha $stored-pattern $pattern)
 *               (=alpha (quote ($stored-extractor $stored-init)) (quote ($extractor $init))))
 *          $stored-id (empty)))
 * PeTTa evaluates =alpha's operands (outside quote) to values first; quote keeps its contents as written;
 * =alpha is CeTTa's atom_alpha_eq (one variable bijection per comparison); and is the conjunction of two
 * Bools; the answer $stored-id is evaluated as a template. */
enum { FBR_TRIGGER, FBR_KBID, FBR_FUN, FBR_FINISH, FBR_TVMODE, FBR_PATTERN, FBR_EXTRACTOR, FBR_INIT,
       FBR_SID, FBR_SPATTERN, FBR_SEXTRACTOR, FBR_SINIT };

static bool s1_value(Space *self, const Atom *x, bool vars_ok);
static void s1_trace_value(const char *what, Atom *x);

static int fbr_row(const S1Site *site, Space *self, Arena *a, Atom **v, Atom **answer) {
    (void)site;
    /* (=alpha $stored-pattern $pattern): both operands evaluated, so both must already be values */
    if (!s1_value(self, v[FBR_SPATTERN], true)) {
        s1_trace_value("stored pattern", v[FBR_SPATTERN]);
        return -1;
    }
    if (!s1_value(self, v[FBR_PATTERN], true)) {
        s1_trace_value("pattern", v[FBR_PATTERN]);
        return -1;
    }
    bool c1 = atom_alpha_eq(v[FBR_SPATTERN], v[FBR_PATTERN]);
    /* (=alpha (quote (se si)) (quote (ex in))): quote's contents are not evaluated */
    Atom *l[2] = {v[FBR_SEXTRACTOR], v[FBR_SINIT]}, *r[2] = {v[FBR_EXTRACTOR], v[FBR_INIT]};
    Atom *lt = atom_expr(a, l, 2u), *rt = atom_expr(a, r, 2u);
    if (!lt || !rt)
        return -1;
    bool c2 = atom_alpha_eq(lt, rt);
    if (!(c1 && c2))
        return 0;
    /* the template $stored-id is evaluated: it must be a ground value */
    if (!s1_value(self, v[FBR_SID], false)) {
        s1_trace_value("stored id", v[FBR_SID]);
        return -1;
    }
    *answer = v[FBR_SID];
    return 1;
}

static S1Site g_sites[] = {
    {
        .name = "fwd_register_base_rate_interest",
        .function = "forward-register-base-rate!",
        .equation_text = S1_EQ_FORWARD_REGISTER_BASE_RATE,
        .space = "&forward_base_rate_interest",
        .input_vars = {"$trigger", "$kbid", "$fun", "$finish", "$tv-mode", "$pattern", "$extractor", "$init"},
        .row_vars = {"$stored-id", "$stored-pattern", "$stored-extractor", "$stored-init"},
        .builtins = {"=alpha", "and", "if", "quote", "empty", "match", "once", "collapse"},
        .row = fbr_row,
    },
};
#define S1_NSITES (sizeof(g_sites) / sizeof(g_sites[0]))

/* ---- helpers ------------------------------------------------------------------------------------------- */

/* A value as PeTTa observes it: internal list carriers (open cons cells, e.g. the chainer's computed patterns)
 * reified as the flat expressions they stand for (petta_semantics_materialize_value, CeTTa's own observation);
 * a carrier with an unresolved tail stays a cons cell, which s1_value then refuses. */
static Atom *s1_observe(Arena *a, Atom *x) {
    if (!x || !petta_semantics_value_contains_observable_open_cons(x))
        return x;
    return petta_semantics_materialize_value(a, x);
}

static void s1_trace_value(const char *what, Atom *x) {
    if (g_s1_trace < 2)
        return;
    fprintf(stderr, "s1-site value check failed on the %s: ", what);
    atom_print(x, stderr);
    fputc('\n', stderr);
}

/* A value PeTTa's evaluation would leave as it is (fnspec's nf check; variables allowed when vars_ok). */
static bool s1_value(Space *self, const Atom *x, bool vars_ok) {
    if (!x)
        return false;
    switch (x->kind) {
    case ATOM_VAR:
        return vars_ok;
    case ATOM_SYMBOL:
        return !space_equations_may_match_known_head(self, x->sym_id);
    case ATOM_GROUNDED:
        return x->ground.gkind == GV_INT || x->ground.gkind == GV_FLOAT || x->ground.gkind == GV_STRING ||
               x->ground.gkind == GV_BOOL;
    case ATOM_EXPR: {
        if (x->expr.len == 0)
            return true;
        const Atom *h = x->expr.elems[0];
        if (h->kind == ATOM_SYMBOL) {
            if (is_grounded_op(h->sym_id) || symbol_id_is_builtin(h->sym_id))
                return false;
        } else if (h->kind == ATOM_VAR) {
            return false;
        } else if (h->kind == ATOM_GROUNDED) {
            GroundedKind g = h->ground.gkind;
            if (g != GV_INT && g != GV_FLOAT && g != GV_STRING && g != GV_BOOL)
                return false;
        } else if (space_equations_may_match_unnamed_head(self)) {
            return false;
        }
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!s1_value(self, x->expr.elems[i], vars_ok))
                return false;
        return true;
    }
    }
    return false;
}

/* alpha-equality with variables by a bijection */
typedef struct { VarId a[256], b[256]; unsigned n; } S1VarMap;
static bool s1_alpha(const Atom *x, const Atom *y, S1VarMap *m) {
    if (x->kind != y->kind)
        return false;
    if (x->kind == ATOM_VAR) {
        for (unsigned i = 0; i < m->n; i++) {
            bool ea = var_id_eq(m->a[i], x->var_id), eb = var_id_eq(m->b[i], y->var_id);
            if (ea || eb)
                return ea && eb;
        }
        if (m->n == 256)
            return false;
        m->a[m->n] = x->var_id;
        m->b[m->n] = y->var_id;
        m->n++;
        return true;
    }
    if (x->kind == ATOM_EXPR) {
        if (x->expr.len != y->expr.len)
            return false;
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!s1_alpha(x->expr.elems[i], y->expr.elems[i], m))
                return false;
        return true;
    }
    return atom_eq((Atom *)x, (Atom *)y);
}

static Atom *s1_find_match(Atom *x, SymbolId space_sym, unsigned depth) {
    if (!x || x->kind != ATOM_EXPR || depth > 64)
        return NULL;
    if (x->expr.len == 4 && x->expr.elems[0]->kind == ATOM_SYMBOL &&
        x->expr.elems[0]->sym_id == g_builtin_syms.match && x->expr.elems[1]->kind == ATOM_SYMBOL &&
        x->expr.elems[1]->sym_id == space_sym)
        return x;
    for (CettaExprLen i = 0; i < x->expr.len; i++) {
        Atom *m = s1_find_match(x->expr.elems[i], space_sym, depth + 1);
        if (m)
            return m;
    }
    return NULL;
}

/* the VarId of a named variable in the parsed equation: parse "$name" alone gives a different id, so the ids
 * are read by walking the equation and matching the printed names */
static bool s1_collect_var(Atom *x, const char *name, VarId *out, unsigned depth) {
    if (!x || depth > 128)
        return false;
    if (x->kind == ATOM_VAR) {
        char *text = NULL;
        size_t len = 0;
        FILE *mem = open_memstream(&text, &len);
        if (!mem)
            return false;
        atom_print(x, mem);
        fclose(mem);
        bool same = text && !strcmp(text, name);
        free(text);
        if (same) {
            *out = x->var_id;
            return true;
        }
        return false;
    }
    if (x->kind == ATOM_EXPR)
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (s1_collect_var(x->expr.elems[i], name, out, depth + 1))
                return true;
    return false;
}

static void s1_parse(S1Site *s) {
    s->parsed = true;
    if (!g_s1_arena_init) {
        arena_init_detached(&g_s1_arena);
        g_s1_arena_init = true;
    }
    size_t pos = 0;
    s->equation = parse_sexpr(&g_s1_arena, s->equation_text, &pos);
    s->space_sym = symbol_intern_cstr(g_symbols, s->space);
    Atom *m = s1_find_match(s->equation, s->space_sym, 0);
    if (!s->equation || !m) {
        s->parse_failed = true;
        return;
    }
    s->pattern = m->expr.elems[2];
    s->template_atom = m->expr.elems[3];
    s->nvars = 0;
    for (int pass = 0; pass < 2; pass++) {
        const char *const *names = pass ? s->row_vars : s->input_vars;
        for (unsigned i = 0; i < S1_MAX_VARS && names[i]; i++) {
            if (s->nvars == S1_MAX_VARS || !s1_collect_var(m, names[i], &s->var_ids[s->nvars], 0)) {
                s->parse_failed = true;
                return;
            }
            s->var_names[s->nvars] = names[i];
            s->var_is_row[s->nvars] = pass == 1;
            s->nvars++;
        }
    }
    /* the definition query (= (f $q1 ... $qn) $qbody) on &self */
    Atom *lhs = s->equation->expr.elems[1];
    if (s->equation->kind != ATOM_EXPR || s->equation->expr.len != 3 || lhs->kind != ATOM_EXPR) {
        s->parse_failed = true;
        return;
    }
    char buf[512];
    int n = snprintf(buf, sizeof(buf), "(= (%s", s->function);
    for (CettaExprLen i = 1; i < lhs->expr.len && n < (int)sizeof(buf) - 32; i++)
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, " $cpk_s1_q%u", (unsigned)i);
    snprintf(buf + n, sizeof(buf) - (size_t)n, ") $cpk_s1_body)");
    pos = 0;
    s->lhs_pattern_query = parse_sexpr(&g_s1_arena, buf, &pos);
    if (!s->lhs_pattern_query)
        s->parse_failed = true;
}

static int s1_var_index(const S1Site *s, VarId id) {
    for (unsigned i = 0; i < s->nvars; i++)
        if (var_id_eq(s->var_ids[i], id))
            return (int)i;
    return -1;
}

/* Is the call's (pattern, template) the site's, up to renaming?  Site variables: a row variable must meet a
 * variable (one bijection); an input variable may meet anything, the same thing at every occurrence. */
typedef struct { Atom *raw[S1_MAX_VARS]; S1VarMap rowmap; } S1Recog;
static bool s1_recognise(const S1Site *s, const Atom *site_x, Atom *raw, S1Recog *r, unsigned depth) {
    if (depth > 128)
        return false;
    if (site_x->kind == ATOM_VAR) {
        int k = s1_var_index(s, site_x->var_id);
        if (k < 0)
            return false; /* a variable the site does not name: not compiled */
        if (s->var_is_row[k]) {
            if (raw->kind != ATOM_VAR)
                return false;
            if (!s1_alpha(site_x, raw, &r->rowmap))
                return false;
            r->raw[k] = raw;
            return true;
        }
        if (!r->raw[k]) {
            r->raw[k] = raw;
            return true;
        }
        return r->raw[k] == raw || atom_eq(r->raw[k], raw);
    }
    if (site_x->kind == ATOM_EXPR) {
        if (raw->kind != ATOM_EXPR || raw->expr.len != site_x->expr.len)
            return false;
        for (CettaExprLen i = 0; i < site_x->expr.len; i++)
            if (!s1_recognise(s, site_x->expr.elems[i], raw->expr.elems[i], r, depth + 1))
                return false;
        return true;
    }
    return atom_eq((Atom *)site_x, raw);
}

static void s1_revalidate(S1Site *s, Space *self) {
    s->st.revalidations++;
    s->valid = false;
    s->tok = space_program_token(self);
    s->tok_valid = true;
    if (space_equations_may_match_unnamed_head(self)) {
        s->invalid_reason = "catch-all equation in &self";
        return;
    }
    for (unsigned i = 0; i < 16 && s->builtins[i]; i++) {
        SymbolId b = symbol_intern_cstr(g_symbols, s->builtins[i]);
        if (space_equations_may_match_known_head(self, b)) {
            s->invalid_reason = "a builtin the template uses has an equation in &self";
            return;
        }
    }
    Arena a;
    arena_init_detached(&a);
    Bindings seed;
    bindings_init(&seed);
    BindingSet rows;
    Atom *q = s->lhs_pattern_query;
    space_query_conjunction(self, &a, &q, 1, &seed, &rows);
    bool same = false;
    if (rows.len == 1) {
        Atom *eq = bindings_apply(&rows.items[0], &a, q);
        S1VarMap m = {.n = 0};
        same = eq && s1_alpha(eq, s->equation, &m);
    }
    CettaCount n = rows.len;
    binding_set_free(&rows);
    bindings_free(&seed);
    arena_free(&a);
    if (n != 1) {
        s->invalid_reason = n ? "more than one equation of the function" : "no equation of the function";
        return;
    }
    if (!same) {
        s->invalid_reason = "the function's equation differs from the compiled one";
        return;
    }
    s->valid = true;
    s->invalid_reason = NULL;
}

static void s1_stats(void) {
    const char *path = getenv("CETTA_S1_SITES_STATS");
    if (!path || !*path)
        return;
    FILE *out = fopen(path, "w");
    if (!out)
        return;
    fprintf(out, "{\"schema\":\"cettapack-s1-sites-v1\",\"mode\":%d,\"sites\":[", g_s1_mode);
    for (size_t i = 0; i < S1_NSITES; i++) {
        S1Site *s = &g_sites[i];
        fprintf(out,
                "%s\n{\"site\":\"%s\",\"function\":\"%s\",\"parsed\":%s,\"valid\":%s,\"invalid_reason\":\"%s\","
                "\"offered\":%" PRIu64 ",\"recognised\":%" PRIu64 ",\"specialised\":%" PRIu64
                ",\"answers\":%" PRIu64 ",\"rows\":%" PRIu64 ",\"revalidations\":%" PRIu64
                ",\"residual\":{\"invalid\":%" PRIu64 ",\"forced\":%" PRIu64 ",\"inputs\":%" PRIu64
                ",\"rowvar\":%" PRIu64 ",\"value\":%" PRIu64 ",\"order\":%" PRIu64 ",\"space\":%" PRIu64
                ",\"query\":%" PRIu64 "}}",
                i ? "," : "", s->name, s->function, s->parse_failed ? "false" : "true", s->valid ? "true" : "false",
                s->invalid_reason ? s->invalid_reason : "", s->st.offered, s->st.recognised, s->st.specialised,
                s->st.answers, s->st.rows, s->st.revalidations, s->st.res_invalid, s->st.res_forced,
                s->st.res_inputs, s->st.res_rowvar, s->st.res_value, s->st.res_order, s->st.res_space,
                s->st.res_query);
    }
    fputs("]}\n", out);
    fclose(out);
}

bool cetta_s1_sites_active(void) {
    if (g_s1_mode < 0) {
        const char *e = getenv("CETTA_S1_SITES");
        g_s1_mode = !e || !*e || !strcmp(e, "0") ? 0 : !strcmp(e, "invalid") ? 2 : 1;
        const char *t = getenv("CETTA_S1_SITES_TRACE");
        g_s1_trace = t && *t && strcmp(t, "0") ? (strcmp(t, "2") ? 1 : 2) : 0;
        if (g_s1_mode)
            atexit(s1_stats);
    }
    return g_s1_mode != 0;
}

/* ---- the call ------------------------------------------------------------------------------------------- */

int cetta_s1_sites_try(const CettaS1SiteCall *call, Atom *reference, Atom *pattern, Atom *template_atom,
                       Atom ***answers, size_t *count) {
    if (!call || !reference || reference->kind != ATOM_SYMBOL || !pattern || !template_atom)
        return CETTA_S1_SITE_DECLINED;
    for (size_t i = 0; i < S1_NSITES; i++) {
        S1Site *s = &g_sites[i];
        if (!s->parsed)
            s1_parse(s);
        if (s->parse_failed || reference->sym_id != s->space_sym)
            continue;
        s->st.offered++;
        S1Recog r;
        memset(&r, 0, sizeof(r));
        if (!s1_recognise(s, s->pattern, pattern, &r, 0) || !s1_recognise(s, s->template_atom, template_atom, &r, 0))
            continue;
        s->st.recognised++;
        if (g_s1_mode == 2) {
            s->st.res_forced++;
            S1_TRACE(s, "residual (forced invalid)");
            return CETTA_S1_SITE_DECLINED;
        }
        if (!s->tok_valid || !space_program_token_is_current(s->tok))
            s1_revalidate(s, call->self);
        if (!s->valid) {
            s->st.res_invalid++;
            S1_TRACE(s, s->invalid_reason ? s->invalid_reason : "invalid");
            return CETTA_S1_SITE_DECLINED;
        }
        /* inputs: the values of the input variables; those in the pattern must be ground */
        Atom *vals[S1_MAX_VARS] = {0};
        for (unsigned k = 0; k < s->nvars; k++) {
            if (!r.raw[k]) {
                s->st.res_inputs++;
                return CETTA_S1_SITE_DECLINED;
            }
            Atom *v = call->apply(call->ctx, r.raw[k]);
            if (v && !s->var_is_row[k])
                v = s1_observe(call->arena, v);
            if (!v) {
                s->st.res_inputs++;
                return CETTA_S1_SITE_DECLINED;
            }
            if (s->var_is_row[k] && v->kind != ATOM_VAR) {
                s->st.res_rowvar++;
                S1_TRACE(s, "residual (a row variable is bound)");
                return CETTA_S1_SITE_DECLINED;
            }
            vals[k] = v;
        }
        Atom *query = call->apply(call->ctx, pattern);
        if (query)
            query = s1_observe(call->arena, query);
        if (!query || petta_semantics_contains_cons_constraint(query)) {
            s->st.res_inputs++;
            S1_TRACE(s, "residual (pattern holds a list carrier the query cannot read)");
            return CETTA_S1_SITE_DECLINED;
        }
        /* every variable left in the instantiated pattern must be one of the row variables */
        {
            Atom *rowv[S1_MAX_VARS];
            unsigned nrow = 0;
            for (unsigned k = 0; k < s->nvars; k++)
                if (s->var_is_row[k])
                    rowv[nrow++] = vals[k];
            /* walk the query */
            Atom *stack[1024];
            unsigned sp = 0;
            stack[sp++] = query;
            bool ok = true;
            while (sp && ok) {
                Atom *x = stack[--sp];
                if (x->kind == ATOM_VAR) {
                    bool known = false;
                    for (unsigned j = 0; j < nrow; j++)
                        if (var_id_eq(rowv[j]->var_id, x->var_id))
                            known = true;
                    ok = known;
                } else if (x->kind == ATOM_EXPR) {
                    for (CettaExprLen j = 0; j < x->expr.len && ok; j++) {
                        if (sp == 1024)
                            ok = false;
                        else
                            stack[sp++] = x->expr.elems[j];
                    }
                }
            }
            if (!ok) {
                s->st.res_inputs++;
                S1_TRACE(s, "residual (pattern inputs not ground)");
                return CETTA_S1_SITE_DECLINED;
            }
        }
        Space *target = call->resolve(call->ctx, reference);
        if (!target) {
            s->st.res_space++;
            return CETTA_S1_SITE_DECLINED;
        }
        Bindings seed;
        bindings_init(&seed);
        BindingSet rows;
        space_query_conjunction(target, call->arena, &query, 1, &seed, &rows);
        bindings_free(&seed);
        Atom **out = NULL;
        size_t nout = 0, cap = 0;
        int verdict = 0; /* 0 ok, -1 value, -2 order, -3 alloc */
        for (CettaCount j = 0; j < rows.len && verdict == 0; j++) {
            s->st.rows++;
            Atom *rv[S1_MAX_VARS];
            for (unsigned k = 0; k < s->nvars; k++) {
                rv[k] = s->var_is_row[k] ? s1_observe(call->arena, bindings_apply(&rows.items[j], call->arena, vals[k]))
                                         : vals[k];
                if (!rv[k])
                    verdict = -3;
            }
            if (verdict)
                break;
            Atom *ans = NULL;
            int k = s->row(s, call->self, call->arena, rv, &ans);
            if (k < 0) {
                verdict = -1;
                break;
            }
            if (k == 0)
                continue;
            if (nout && !atom_eq(out[0], ans)) {
                verdict = -2;
                break;
            }
            if (nout == cap) {
                cap = cap ? cap * 2 : 4;
                Atom **t = realloc(out, cap * sizeof(*t));
                if (!t) {
                    verdict = -3;
                    break;
                }
                out = t;
            }
            out[nout++] = ans;
        }
        binding_set_free(&rows);
        if (verdict) {
            free(out);
            if (verdict == -1)
                s->st.res_value++;
            else if (verdict == -2)
                s->st.res_order++;
            else
                s->st.res_query++;
            S1_TRACE(s, verdict == -1 ? "residual (a value the template evaluates is not a value)"
                        : verdict == -2 ? "residual (more than one distinct answer: row order)" : "residual (query)");
            return CETTA_S1_SITE_DECLINED;
        }
        s->st.specialised++;
        s->st.answers += nout;
        S1_TRACE(s, nout ? "specialised (answered)" : "specialised (no answer)");
        *answers = out;
        *count = nout;
        return CETTA_S1_SITE_ANSWERS;
    }
    return CETTA_S1_SITE_DECLINED;
}
#endif
