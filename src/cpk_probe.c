/* cettapack-probe: the three next-step probes (t-2026-11-18-cettapack-next-step-probe).
 *
 *   cettapack-probe-<site> bench --kb FILE --queries FILE --mode MODE [--repeat N]
 *   cettapack-probe-<site> difftest [--kb FILE --queries FILE]
 *   cettapack-probe-<site> defs
 *
 * Modes (every query atom is built before the timed loop, in every mode):
 *   generic         (match S <conj> <tmpl>)                      evaluated match
 *   nested          (match S f1 (match S f2 ... <tmpl>))         probe 2 baseline: the same
 *                                                                matches one after another
 *   direct          (match-rows S <conj> <tmpl>)                 probe 1: CeTTa builtin from
 *                                                                upstream/cetta/0003
 *   hosted-generic  (<site> S seed), CeTTa's conjunction matcher behind the site callable
 *   kernel          (<site> S seed), the compiled kernel (guard, PathMap narrowing, rematch);
 *                   for probe 2 this is the fused kernel of the nested matches
 *   fallback        (<site> S seed), guard INVALID on every call -> residual
 *   fn-generic      (cpk-fn S seed): a MeTTa function whose body is the site's match with
 *                   a filter, (if (== <var> tissue0) (empty) <tmpl>), evaluated by CeTTa
 *   fn-hosted       (cpk-fn-h S seed): the same function with its match replaced by the
 *                   hosted-generic site callable (CeTTa's matcher), the rest evaluated by CeTTa
 *   fn-kernel       the same, with the compiled kernel as the site callable
 *   fn-spec         (cpk-fnspec S seed): the whole function specialised into one callable:
 *                   guard, CeTTa's conjunction matcher, the filter and the template in C
 *   fn-fallback     cpk-fnspec with its guard forced INVALID -> residual (the MeTTa function)
 */
#define _POSIX_C_SOURCE 200809L
#include "cetta_embed.h"
#include "cetta_embed_native.h"
#include "cpk_pack.h"
#include "match.h"
#include "space_match_backend.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#ifndef CPK_PLAN_SYM
#define CPK_PLAN_SYM cpk_plan_cpk_site_target_expressed
#endif
extern const CpkPlan CPK_PLAN_SYM;
#define PLAN (&CPK_PLAN_SYM)
#ifndef CPK_PACK_DIR
#define CPK_PACK_DIR "packs/target_expressed"
#endif
static const char *g_manifest = CPK_PACK_DIR "/manifest.json";
static const char *g_site = CPK_PACK_DIR "/site.metta";

/* the probe-3 filter: the template's last variable compared with a constant */
#define FN_FILTER_CONST "tissue0"

static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "cettapack-probe: "); vfprintf(stderr, fmt, ap); fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static cetta_engine_t *engine_new(void) {
    cetta_engine_config_t cfg = {.size = sizeof cfg, .abi_version = CETTA_EMBED_ABI_VERSION,
                                 .language = "he", .resource_root = CPK_CETTA_ROOT};
    cetta_engine_t *e = NULL;
    cetta_status_t st = cetta_embed_engine_new(&cfg, &e);
    if (st != CETTA_OK) die("engine: %s", cetta_embed_status_name(st));
    return e;
}

static cetta_atom_t *parse1(cetta_engine_t *e, const char *text) {
    cetta_atom_t *a = NULL;
    cetta_status_t st = cetta_embed_parse(e, text, strlen(text), &a);
    if (st != CETTA_OK) die("parse '%s': %s", text, cetta_embed_status_name(st));
    return a;
}
static cetta_atom_t *sym(cetta_engine_t *e, const char *s) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_atom_symbol(e, s, &a) != CETTA_OK) die("symbol");
    return a;
}
static cetta_atom_t *child(const cetta_atom_t *a, size_t i) {
    cetta_atom_t *c = NULL;
    if (cetta_embed_atom_child(a, i, &c) != CETTA_OK) die("child");
    return c;
}
static cetta_atom_t *expr(cetta_engine_t *e, cetta_atom_t **items, size_t n) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_atom_expression(e, items, n, &a) != CETTA_OK) die("expression");
    return a;
}
static char *fmt(const cetta_atom_t *a) {
    size_t need = 0;
    cetta_embed_atom_format(a, NULL, 0, &need);
    char *s = malloc(need + 1);
    if (!s || cetta_embed_atom_format(a, s, need + 1, &need) != CETTA_OK) die("format");
    return s;
}

/* text with every occurrence of %1$s replaced by rep */
static char *subst(const char *f, const char *rep) {
    size_t n = strlen(f) + 1, rl = strlen(rep);
    for (const char *p = f; (p = strstr(p, "%1$s")); p += 4) n += rl;
    char *out = malloc(n), *o = out;
    for (const char *p = f; *p;) {
        if (!strncmp(p, "%1$s", 4)) { memcpy(o, rep, rl); o += rl; p += 4; }
        else *o++ = *p++;
    }
    *o = 0;
    return out;
}

/* the site's conjunction and template text, from the plan's generic form "(_ <conj> <tmpl>)" */
static void site_text(const char *seed, char **conj, char **tmpl) {
    char *g = subst(PLAN->generic_fmt, seed);
    const char *p = g + 3;
    int depth = 0; const char *q = p;
    do { if (*q == '(') depth++; else if (*q == ')') depth--; q++; } while (depth > 0 && *q);
    *conj = strndup(p, (size_t)(q - p));
    while (*q == ' ') q++;
    *tmpl = strndup(q, strlen(q) - 1);   /* drop the closing ')' of "(_ ...)" */
    free(g);
}

/* the variable the probe-3 filter tests: the template's last variable */
static char *filter_var(const char *tmpl) {
    const char *d = strrchr(tmpl, '$');
    if (!d) die("template has no variable: %s", tmpl);
    const char *e = d; while (*e && *e != ' ' && *e != ')') e++;
    return strndup(d, (size_t)(e - d));
}

/* ---- bags ------------------------------------------------------------------ */

typedef struct { char **items; size_t n; } Bag;
static int cmp_s(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static void bag_free(Bag *b) { for (size_t i = 0; i < b->n; i++) free(b->items[i]); free(b->items); b->items = NULL; b->n = 0; }
static bool g_eval_error;
static Bag eval_bag(cetta_engine_t *e, cetta_atom_t *x) {
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_eval(e, x, NULL, &r);
    if (st == CETTA_OK && cetta_embed_result_status(r) != CETTA_OK) st = cetta_embed_result_status(r);
    if (st != CETTA_OK) {
        g_eval_error = true;
        cetta_embed_result_release(r);
        Bag b = {calloc(1, sizeof(char *)), 1};
        b.items[0] = strdup("<eval error>");
        return b;
    }
    size_t n = cetta_embed_result_count(r);
    Bag b = {calloc(n + 1, sizeof(char *)), n};
    for (size_t i = 0; i < n; i++) {
        cetta_atom_t *h = NULL;
        cetta_embed_result_get(r, i, &h);
        b.items[i] = fmt(h);
        cetta_embed_atom_release(h);
    }
    cetta_embed_result_release(r);
    qsort(b.items, b.n, sizeof(char *), cmp_s);
    return b;
}
static bool bag_eq(const Bag *a, const Bag *b) {
    if (a->n != b->n) return false;
    for (size_t i = 0; i < a->n; i++) if (strcmp(a->items[i], b->items[i])) return false;
    return true;
}
static void show_bag(const char *label, const Bag *b) {
    fprintf(stderr, "    %s (%zu):", label, b->n);
    for (size_t i = 0; i < b->n && i < 10; i++) fprintf(stderr, " %s", b->items[i]);
    fprintf(stderr, "%s\n", b->n > 10 ? " ..." : "");
}

/* ---- spaces ---------------------------------------------------------------- */

static cetta_atom_t *new_space(cetta_engine_t *e) {
    cetta_space_t *s = NULL;
    if (cetta_embed_space_new(e, &s) != CETTA_OK) die("new space");
    cetta_atom_t *a = NULL;
    cetta_embed_space_atom(s, &a);
    cetta_embed_space_release(s);
    return a;
}
static void add_text(cetta_engine_t *e, cetta_atom_t *space, const char *text) {
    cetta_space_t *s = NULL;
    if (cetta_embed_space_from_atom(e, space, &s) != CETTA_OK) die("space handle");
    const char *p = text;
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t n = nl ? (size_t)(nl - p) : strlen(p);
        if (n > 0 && p[0] != ';') {
            char *line = strndup(p, n);
            cetta_atom_t *a = parse1(e, line);
            if (cetta_embed_space_add_atom(s, a) != CETTA_OK) die("add %s", line);
            cetta_embed_atom_release(a);
            free(line);
        }
        p += n + (nl ? 1 : 0);
    }
    cetta_embed_space_release(s);
}

/* route record (the stats-off facts of docs/ROUTE_CHECK.md): the match backend
 * CeTTa holds for the space and its native row storage */
static void route_json(cetta_atom_t *space, char *out, size_t len) {
    Space *s = cetta_embed_native_space(space);
    if (!s) { snprintf(out, len, "{\"error\":\"no native space\"}"); return; }
    snprintf(out, len, "{\"backend\":\"%s\",\"native_rows\":%llu,\"dense_facts\":%llu}",
             space_match_backend_kind_name(s->match_backend.kind), (unsigned long long)s->native.len,
             (unsigned long long)s->native.dense_facts);
}

/* ---- probe 3: the specialised function -------------------------------------- */

typedef struct {
    cetta_engine_t *engine;
    cetta_atom_t *body_handle;
    Atom *conj, *tmpl, *seed_var, *filter_var, *filter_const;
    bool def_ok;                   /* &self holds exactly the compiled definition */
    bool force_fallback;           /* fn-fallback: guard INVALID on every call */
    uint64_t calls, spec_calls, fallbacks, aborts;
    cetta_atom_t *callable;
} FnSpec;

typedef struct {
    FnSpec *f;
    cetta_atom_t *const *args;
    cetta_atom_t **answers;
    size_t n, cap, pos;
    bool computed, failed;
} FnCursor;

static int fc_push(FnCursor *c, cetta_atom_t *h) {
    if (c->n == c->cap) {
        size_t cap = c->cap ? c->cap * 2 : 16;
        cetta_atom_t **t = realloc(c->answers, cap * sizeof(*t));
        if (!t) return -1;
        c->answers = t; c->cap = cap;
    }
    c->answers[c->n++] = h;
    return 0;
}

/* residual: the MeTTa function itself, (cpk-fn S seed), evaluated by CeTTa */
static bool fn_residual(FnCursor *c) {
    FnSpec *f = c->f;
    f->fallbacks++;
    cetta_atom_t *items[3] = {sym(f->engine, "cpk-fn"), c->args[0], c->args[1]};
    cetta_atom_t *x = expr(f->engine, items, 3);
    cetta_embed_atom_release(items[0]);
    cetta_result_t *r = NULL;
    bool ok = cetta_embed_eval(f->engine, x, NULL, &r) == CETTA_OK && cetta_embed_result_status(r) == CETTA_OK;
    for (size_t i = 0; ok && i < cetta_embed_result_count(r); i++) {
        cetta_atom_t *h = NULL;
        ok = cetta_embed_result_get(r, i, &h) == CETTA_OK && fc_push(c, h) == 0;
    }
    cetta_embed_result_release(r);
    cetta_embed_atom_release(x);
    return ok;
}

static void fn_compute(FnCursor *c) {
    FnSpec *f = c->f;
    f->calls++;
    Space *s = cetta_embed_native_space(c->args[0]);
    Atom *seed = (Atom *)cetta_embed_native_atom(c->args[1]);
    /* guard: the compiled definition is the live one, a space, a symbol seed */
    bool valid = !f->force_fallback && f->def_ok && s && seed && seed->kind == ATOM_SYMBOL;
    if (valid) {
        Arena a;
        arena_init_detached(&a);
        Bindings sb;
        bindings_init(&sb);
        bool ok = bindings_add_var(&sb, f->seed_var, seed);
        Atom **raw = NULL; size_t nraw = 0, rawcap = 0, filtered = 0;
        if (ok) {
            BindingSet rows;
            space_query_conjunction(s, &a, f->conj->expr.elems + 1, f->conj->expr.len - 1, &sb, &rows);
            for (CettaCount i = 0; ok && i < rows.len; i++) {
                Atom *v = bindings_apply(&rows.items[i], &a, f->filter_var);
                /* the C filter equals (== v tissue0) for symbol values only; any
                 * other value is outside the compiled fragment: abort to the residual */
                if (!v || v->kind != ATOM_SYMBOL) { ok = false; break; }
                if (atom_eq(v, f->filter_const)) { filtered++; continue; }   /* the (empty) branch */
                Atom *ans = bindings_apply(&rows.items[i], &a, f->tmpl);
                if (!ans) { ok = false; break; }
                if (nraw == rawcap) {
                    rawcap = rawcap ? rawcap * 2 : 16;
                    Atom **t = realloc(raw, rawcap * sizeof(*t));
                    if (!t) { ok = false; break; }
                    raw = t;
                }
                raw[nraw++] = ans;
            }
            binding_set_free(&rows);
        }
        /* CeTTa's observable result for this body (bag tests, d5/d6): when every
         * row takes the (empty) branch, the call returns one Empty per row; when
         * any row gives a value, the Empty results are dropped */
        if (ok && nraw == 0 && filtered > 0) {
            Atom *empty = atom_symbol(&a, "Empty");
            raw = malloc(filtered * sizeof(*raw));
            ok = raw != NULL;
            for (size_t i = 0; ok && i < filtered; i++) raw[nraw++] = empty;
        }
        if (ok && nraw) {
            cetta_atom_t *bundle = NULL;
            ok = cetta_embed_native_atom_copy(f->engine, atom_expr(&a, raw, nraw), &bundle) == CETTA_OK;
            for (size_t i = 0; ok && i < nraw; i++) {
                cetta_atom_t *h = NULL;
                ok = cetta_embed_atom_child(bundle, i, &h) == CETTA_OK && fc_push(c, h) == 0;
            }
            cetta_embed_atom_release(bundle);
        }
        free(raw);
        bindings_free(&sb);
        arena_free(&a);
        if (ok) { f->spec_calls++; return; }
        for (size_t i = 0; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
        c->n = 0;
        f->aborts++;
    }
    if (!fn_residual(c)) {
        for (size_t i = 0; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
        c->n = 0;
        c->failed = true;
    }
}

static cetta_status_t fn_open(void *ctx, cetta_engine_t *engine, cetta_atom_t *const *args, size_t arity, void **out) {
    (void)engine; (void)arity;
    FnCursor *c = calloc(1, sizeof(*c));
    if (!c) return CETTA_OUT_OF_MEMORY;
    c->f = ctx; c->args = args;
    *out = c;
    return CETTA_OK;
}
static cetta_status_t fn_next(void *cursor, cetta_callback_state_t *state, cetta_atom_t **out) {
    FnCursor *c = cursor;
    *out = NULL;
    if (!c->computed) { fn_compute(c); c->computed = true; }
    if (c->failed) { *state = CETTA_CALLBACK_COMPLETE; return CETTA_ENGINE_ERROR; }
    if (c->pos < c->n) { *out = c->answers[c->pos++]; *state = CETTA_CALLBACK_VALUE; return CETTA_OK; }
    *state = CETTA_CALLBACK_COMPLETE;
    return CETTA_OK;
}
static void fn_close(void *cursor, cetta_callback_close_mode_t mode) {
    (void)mode;
    FnCursor *c = cursor;
    for (size_t i = c->pos; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
    free(c->answers);
    free(c);
}
static void fn_nop(void *ctx) { (void)ctx; }

/* the MeTTa definitions: the function, and the same function on the site callable */
static char *fn_defs(void) {
    char *conj, *tmpl;
    site_text("$seed", &conj, &tmpl);
    char *v = filter_var(tmpl);
    size_t n = 4 * strlen(conj) + 4 * strlen(tmpl) + 1024;
    char *t = malloc(n);
    snprintf(t, n,
             "(= (cpk-fn $space $seed) (match $space %s (if (== %s " FN_FILTER_CONST ") (empty) %s)))\n"
             "(= (cpk-fn-h $space $seed) (let %s (%s $space $seed) (if (== %s " FN_FILTER_CONST ") (empty) %s)))\n",
             conj, v, tmpl, tmpl, PLAN->site, v, tmpl);
    free(conj); free(tmpl); free(v);
    return t;
}

static void load_defs(cetta_engine_t *e) {
    char *t = fn_defs();
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_run(e, t, strlen(t), NULL, &r);
    if (st != CETTA_OK || cetta_embed_result_status(r) != CETTA_OK) die("load definitions: %s\n%s", cetta_embed_status_name(st), t);
    cetta_embed_result_release(r);
    free(t);
}

/* canonical text of an atom with variables renamed by first occurrence ($_0, $_1, ...),
 * as cpk_pack.c's structural digest does it */
typedef struct { char *names[64]; unsigned n; } VarNames;
static bool canon_rec(const cetta_atom_t *a, char **buf, size_t *len, VarNames *vn) {
    char text[512] = {0}, piece[600];
    size_t need = 0;
    cetta_atom_kind_t k = cetta_embed_atom_kind(a);
    if (k == CETTA_ATOM_EXPRESSION) {
        size_t n = cetta_embed_atom_arity(a);
        snprintf(piece, sizeof piece, "(");
    #define PUT(s) do { size_t l = strlen(s); char *t = realloc(*buf, *len + l + 1); if (!t) return false; memcpy(t + *len, s, l + 1); *buf = t; *len += l; } while (0)
        PUT(piece);
        for (size_t i = 0; i < n; i++) {
            cetta_atom_t *c = child(a, i);
            if (i) PUT(" ");
            bool ok = canon_rec(c, buf, len, vn);
            cetta_embed_atom_release(c);
            if (!ok) return false;
        }
        PUT(")");
        return true;
    }
    if (k == CETTA_ATOM_VARIABLE) {
        if (cetta_embed_atom_get_text(a, text, sizeof text, &need) != CETTA_OK) return false;
        unsigned idx = vn->n;
        for (unsigned i = 0; i < vn->n; i++) if (!strcmp(vn->names[i], text)) { idx = i; break; }
        if (idx == vn->n) { if (vn->n == 64) return false; vn->names[vn->n++] = strdup(text); }
        snprintf(piece, sizeof piece, "$_%u", idx);
        PUT(piece);
        return true;
    }
    if (cetta_embed_atom_format(a, text, sizeof text, &need) != CETTA_OK) return false;
    PUT(text);
    #undef PUT
    return true;
}
static char *canon_text(const cetta_atom_t *a) {
    char *buf = calloc(1, 1); size_t len = 0;
    VarNames vn = {0};
    bool ok = canon_rec(a, &buf, &len, &vn);
    for (unsigned i = 0; i < vn.n; i++) free(vn.names[i]);
    if (!ok) { free(buf); return NULL; }
    return buf;
}

static FnSpec *fnspec_open(cetta_engine_t *e, bool force_fallback) {
    FnSpec *f = calloc(1, sizeof(*f));
    f->engine = e;
    f->force_fallback = force_fallback;
    char *conj, *tmpl;
    site_text("$seed", &conj, &tmpl);
    char *v = filter_var(tmpl);
    char text[2048];
    snprintf(text, sizeof text, "(_ $seed %s %s %s " FN_FILTER_CONST " (match $space %s (if (== %s " FN_FILTER_CONST ") (empty) %s)))",
             conj, tmpl, v, conj, v, tmpl);
    f->body_handle = parse1(e, text);
    Atom *all = (Atom *)cetta_embed_native_atom(f->body_handle);
    f->seed_var = all->expr.elems[1];
    f->conj = all->expr.elems[2];
    f->tmpl = all->expr.elems[3];
    f->filter_var = all->expr.elems[4];
    f->filter_const = all->expr.elems[5];
    /* the compiled body must be the body of cpk-fn in &self (structural digest,
     * variable names normalised); checked once at open (a full milestone guards
     * it per revision of &self) */
    cetta_atom_t *bh = child(f->body_handle, 6);
    char *want = canon_text(bh), *got = NULL;
    cetta_embed_atom_release(bh);
    cetta_atom_t *q = parse1(e, "(match &self (= (cpk-fn $space $seed) $body) (quote $body))");
    cetta_result_t *r = NULL;
    int nbodies = 0;
    if (cetta_embed_eval(e, q, NULL, &r) == CETTA_OK && cetta_embed_result_status(r) == CETTA_OK) {
        nbodies = (int)cetta_embed_result_count(r);
        if (nbodies == 1) {
            cetta_atom_t *h = NULL;
            cetta_embed_result_get(r, 0, &h);
            if (cetta_embed_atom_arity(h) == 2) {
                cetta_atom_t *b = child(h, 1);
                got = canon_text(b);
                cetta_embed_atom_release(b);
            }
            cetta_embed_atom_release(h);
        }
    }
    cetta_embed_result_release(r);
    cetta_embed_atom_release(q);
    f->def_ok = nbodies == 1 && want && got && !strcmp(want, got);
    if (!f->def_ok) fprintf(stderr, "fnspec: definition check failed (bodies=%d)\n  want %s\n  got  %s\n", nbodies, want ? want : "-", got ? got : "-");
    free(want); free(got);
    free(conj); free(tmpl); free(v);
    cetta_callback_descriptor_t d = {
        .size = sizeof d, .arity = 2, .kind = CETTA_CALLBACK_STREAM, .context = f,
        .retain = fn_nop, .release = fn_nop, .open = fn_open, .next = fn_next, .close = fn_close,
    };
    if (cetta_embed_register_callback(e, "cpk-fnspec", &d, &f->callable) != CETTA_OK) die("register cpk-fnspec");
    return f;
}
static void fnspec_close(FnSpec *f) {
    if (!f) return;
    cetta_embed_atom_release(f->callable);
    cetta_embed_atom_release(f->body_handle);
    free(f);
}

/* ---- query atoms per mode --------------------------------------------------- */

typedef enum { Q_GENERIC, Q_NESTED, Q_DIRECT, Q_SITE, Q_FN, Q_FN_H, Q_FNSPEC } QKind;

static cetta_atom_t *query_atom(cetta_engine_t *e, cetta_atom_t *space, const char *seed, QKind k) {
    char *conj, *tmpl;
    site_text(seed, &conj, &tmpl);
    cetta_atom_t *x = NULL;
    /* conj and tmpl parsed as one atom, so that they share variables */
    char *both = malloc(strlen(conj) + strlen(tmpl) + 8);
    sprintf(both, "(_ %s %s)", conj, tmpl);
    if (k == Q_GENERIC || k == Q_DIRECT) {
        cetta_atom_t *p = parse1(e, both);
        cetta_atom_t *c = child(p, 1), *t = child(p, 2);
        cetta_atom_t *items[4] = {sym(e, k == Q_GENERIC ? "match" : "match-rows"), space, c, t};
        x = expr(e, items, 4);
        cetta_embed_atom_release(items[0]); cetta_embed_atom_release(c); cetta_embed_atom_release(t);
        cetta_embed_atom_release(p);
    } else if (k == Q_NESTED) {
        cetta_atom_t *p = parse1(e, both);
        cetta_atom_t *c = child(p, 1), *body = child(p, 2);
        size_t nf = cetta_embed_atom_arity(c) - 1;
        for (size_t i = nf; i >= 1; i--) {
            cetta_atom_t *f = child(c, i);
            cetta_atom_t *items[4] = {sym(e, "match"), space, f, body};
            cetta_atom_t *m = expr(e, items, 4);
            cetta_embed_atom_release(items[0]); cetta_embed_atom_release(f); cetta_embed_atom_release(body);
            body = m;
        }
        cetta_embed_atom_release(c);
        cetta_embed_atom_release(p);
        x = body;
    } else {
        const char *head = k == Q_SITE ? PLAN->site : k == Q_FN ? "cpk-fn" : k == Q_FN_H ? "cpk-fn-h" : "cpk-fnspec";
        cetta_atom_t *items[3] = {sym(e, head), space, parse1(e, seed)};
        x = expr(e, items, 3);
        cetta_embed_atom_release(items[0]); cetta_embed_atom_release(items[2]);
    }
    free(both); free(conj); free(tmpl);
    return x;
}

typedef struct {
    const char *name;
    QKind q;
    bool pathmap, native_generic, fnspec, fnspec_fallback;
} Mode;

static const Mode MODES[] = {
    {"generic", Q_GENERIC, true, false, false, false},
    {"nested", Q_NESTED, true, false, false, false},
    {"direct", Q_DIRECT, true, false, false, false},
    {"hosted-generic", Q_SITE, true, true, false, false},
    {"kernel", Q_SITE, true, false, false, false},
    {"fallback", Q_SITE, false, false, false, false},
    {"fn-generic", Q_FN, true, false, false, false},
    {"fn-hosted", Q_FN_H, true, true, false, false},
    {"fn-kernel", Q_FN_H, true, false, false, false},
    {"fn-spec", Q_FNSPEC, true, false, true, false},
    {"fn-fallback", Q_FNSPEC, true, false, true, true},
};
static const Mode *find_mode(const char *name) {
    for (size_t i = 0; i < sizeof MODES / sizeof *MODES; i++) if (!strcmp(MODES[i].name, name)) return &MODES[i];
    die("unknown mode %s", name);
    return NULL;
}

typedef struct { cetta_engine_t *e; CpkPack *pack; FnSpec *fn; cetta_atom_t *space; } Env;

static Env env_open(const Mode *m, const char *kbtext) {
    Env v = {0};
    v.e = engine_new();
    CpkPackOptions o = {.plan = PLAN, .manifest_path = g_manifest, .site_path = g_site, .language = "he",
                        .pathmap_enabled = m->pathmap, .demand = CPK_DEMAND_BAG, .native_generic = m->native_generic};
    if (cpk_pack_open(v.e, &o, &v.pack) != CETTA_OK) die("pack open");
    load_defs(v.e);
    if (m->fnspec) v.fn = fnspec_open(v.e, m->fnspec_fallback);
    v.space = new_space(v.e);
    add_text(v.e, v.space, kbtext);
    return v;
}
static void env_close(Env *v) {
    cetta_embed_atom_release(v->space);
    fnspec_close(v->fn);
    cpk_pack_close(v->pack);
    cetta_embed_engine_release(v->e);
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

static int cmd_bench(const char *kbfile, const char *qfile, const char *mode, int repeat) {
    const Mode *m = find_mode(mode);
    char *kb = cpk_read_file(kbfile, NULL), *qs = cpk_read_file(qfile, NULL);
    if (!kb || !qs) die("read inputs");
    char *seeds[4096]; int nq = 0;
    for (char *s = strtok(qs, "\n"); s && nq < 4096; s = strtok(NULL, "\n")) seeds[nq++] = s;
    uint64_t t0 = cpk_now_ns();
    Env v = env_open(m, kb);
    uint64_t load_ns = cpk_now_ns() - t0;
    free(kb);
    cetta_atom_t **qa = calloc((size_t)nq, sizeof(*qa));
    for (int i = 0; i < nq; i++) qa[i] = query_atom(v.e, v.space, seeds[i], m->q);
    char route0[256], route1[256];
    /* one untimed warm-up call in every mode (instantiation; CeTTa's first-match storage move) */
    {
        cetta_result_t *r = NULL;
        if (cetta_embed_eval(v.e, qa[0], NULL, &r) != CETTA_OK) die("warm-up eval");
        cetta_embed_result_release(r);
        memset(&v.pack->stats, 0, sizeof v.pack->stats);
        if (v.fn) v.fn->calls = v.fn->spec_calls = v.fn->fallbacks = v.fn->aborts = 0;
    }
    route_json(v.space, route0, sizeof route0);
    uint64_t *batch = calloc((size_t)repeat, sizeof(uint64_t));
    size_t answers = 0;
    for (int r = 0; r < repeat; r++) {
        uint64_t b0 = cpk_now_ns();
        size_t a = 0;
        for (int i = 0; i < nq; i++) {
            cetta_result_t *res = NULL;
            if (cetta_embed_eval(v.e, qa[i], NULL, &res) != CETTA_OK) die("eval");
            if (cetta_embed_result_status(res) != CETTA_OK) die("eval status %s", cetta_embed_status_name(cetta_embed_result_status(res)));
            a += cetta_embed_result_count(res);
            cetta_embed_result_release(res);
        }
        batch[r] = cpk_now_ns() - b0;
        answers = a;
    }
    route_json(v.space, route1, sizeof route1);
    uint64_t sorted[1024];
    int rr = repeat < 1024 ? repeat : 1024;
    memcpy(sorted, batch, sizeof(uint64_t) * (size_t)rr);
    qsort(sorted, (size_t)rr, sizeof(uint64_t), cmp_u64);
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    printf("{\"mode\":\"%s\",\"site\":\"%s\",\"queries\":%d,\"repeat\":%d,\"answers_per_batch\":%zu,"
           "\"batch_ns_median\":%" PRIu64 ",\"batch_ns_min\":%" PRIu64 ",\"batch_ns_max\":%" PRIu64 ","
           "\"per_query_us_median\":%.3f,\"kb_load_ms\":%.1f,\"index_build_ms\":%.1f,"
           "\"site_calls\":%" PRIu64 ",\"kernel_calls\":%" PRIu64 ",\"fallbacks\":%" PRIu64 ","
           "\"fn_calls\":%" PRIu64 ",\"fn_spec_calls\":%" PRIu64 ",\"fn_fallbacks\":%" PRIu64 ",\"fn_aborts\":%" PRIu64 ","
           "\"fn_def_ok\":%s,\"route_before\":%s,\"route_after\":%s,\"peak_rss_kb\":%ld}\n",
           mode, PLAN->site, nq, repeat, answers, sorted[rr / 2], sorted[0], sorted[rr - 1],
           (double)sorted[rr / 2] / 1000.0 / nq, load_ns / 1e6, v.pack->instantiate_ns / 1e6,
           v.pack->stats.calls, v.pack->stats.kernel_calls, v.pack->stats.fallbacks,
           v.fn ? v.fn->calls : 0, v.fn ? v.fn->spec_calls : 0, v.fn ? v.fn->fallbacks : 0, v.fn ? v.fn->aborts : 0,
           v.fn ? (v.fn->def_ok ? "true" : "false") : "null", route0, route1, ru.ru_maxrss);
    for (int i = 0; i < nq; i++) cetta_embed_atom_release(qa[i]);
    free(qa); free(batch); free(qs);
    env_close(&v);
    return 0;
}

/* ---- differential bag tests ------------------------------------------------- */

/* For every seed, each probe mode's bag against its reference: generic CeTTa's
 * evaluated match for the match modes, CeTTa evaluating the MeTTa function for
 * the fn modes. Each mode runs in its own engine (its own pack options) on its
 * own copy of the same space. Path checks: every kernel call on the kernel
 * path, every fallback call on the residual, every fn-spec call specialised,
 * every fn-fallback call on the residual. */
static void difftest_one(const char *label, const char *kbtext, char **seeds, int ns, int *fails) {
    static const char *match_modes[] = {"nested", "direct", "hosted-generic", "kernel", "fallback"};
    static const char *fn_modes[] = {"fn-hosted", "fn-kernel", "fn-spec", "fn-fallback"};
    Env ref = env_open(find_mode("generic"), kbtext);
    Bag *g = calloc((size_t)ns, sizeof(Bag)), *fg = calloc((size_t)ns, sizeof(Bag));
    size_t ga = 0, fa = 0;
    g_eval_error = false;
    for (int i = 0; i < ns; i++) {
        cetta_atom_t *x = query_atom(ref.e, ref.space, seeds[i], Q_GENERIC), *y = query_atom(ref.e, ref.space, seeds[i], Q_FN);
        g[i] = eval_bag(ref.e, x); fg[i] = eval_bag(ref.e, y);
        ga += g[i].n; fa += fg[i].n;
        cetta_embed_atom_release(x); cetta_embed_atom_release(y);
    }
    if (g_eval_error) { printf("FAIL %s reference evaluation error\n", label); (*fails)++; }
    env_close(&ref);
    for (int pass = 0; pass < 2; pass++) {
        const char **list = pass ? fn_modes : match_modes;
        size_t nl = pass ? 4 : 5;
        for (size_t mi = 0; mi < nl; mi++) {
            const Mode *m = find_mode(list[mi]);
            Env v = env_open(m, kbtext);
            int same = 0, shown = 0;
            g_eval_error = false;
            for (int i = 0; i < ns; i++) {
                cetta_atom_t *x = query_atom(v.e, v.space, seeds[i], m->q);
                Bag b = eval_bag(v.e, x);
                Bag *want = pass ? &fg[i] : &g[i];
                if (bag_eq(want, &b)) same++;
                else if (shown++ < 3) {
                    fprintf(stderr, "  %s %s: mismatch at %s\n", label, m->name, seeds[i]);
                    show_bag("reference", want); show_bag(m->name, &b);
                }
                bag_free(&b);
                cetta_embed_atom_release(x);
            }
            bool path = true;
            CpkStats *st = &v.pack->stats;
            if (!strcmp(m->name, "kernel") || !strcmp(m->name, "fn-kernel")) path = st->kernel_calls == (uint64_t)ns && st->fallbacks == 0;
            else if (!strcmp(m->name, "fallback")) path = st->fallbacks == (uint64_t)ns && st->kernel_calls == 0;
            else if (!strcmp(m->name, "hosted-generic") || !strcmp(m->name, "fn-hosted")) path = st->calls == (uint64_t)ns;
            else if (!strcmp(m->name, "fn-spec")) path = v.fn->def_ok && v.fn->spec_calls == (uint64_t)ns && v.fn->fallbacks == 0;
            else if (!strcmp(m->name, "fn-fallback")) path = v.fn->fallbacks == (uint64_t)ns && v.fn->spec_calls == 0;
            bool ok = same == ns && path && !g_eval_error;
            if (!path && (!strcmp(m->name, "kernel") || !strcmp(m->name, "fn-kernel")))
                fprintf(stderr, "  %s %s: last verdict %s (%s)\n", label, m->name, cpk_verdict_name(v.pack->last_verdict), v.pack->last_reason);
            printf("%s %-6s %-15s seeds=%d same=%d ref_answers=%zu path=%s site_calls=%" PRIu64 " kernel=%" PRIu64
                   " fallbacks=%" PRIu64 " fn_spec=%" PRIu64 " fn_fallbacks=%" PRIu64 "\n",
                   ok ? "PASS" : "FAIL", label, m->name, ns, same, pass ? fa : ga, path ? "ok" : "WRONG",
                   st->calls, st->kernel_calls, st->fallbacks, v.fn ? v.fn->spec_calls : 0, v.fn ? v.fn->fallbacks : 0);
            fflush(stdout);
            if (!ok) (*fails)++;
            env_close(&v);
        }
    }
    for (int i = 0; i < ns; i++) { bag_free(&g[i]); bag_free(&fg[i]); }
    free(g); free(fg);
}

static int cmd_difftest(const char *kbfile, const char *qfile) {
    char *defs = fn_defs();
    printf("site %s; fn definitions:\n%s", PLAN->site, defs);
    free(defs);
    /* the M1-numbers difftest spaces, plus d5, where the filter constant occurs */
    static const char *spaces[][2] = {
        {"d1", "(target rs1 g1)\n(target rs1 g2)\n(target rs2 g3)\n(target rs9 g9)\n(expressed g1 liver)\n(expressed g1 brain)\n"
               "(expressed g2 heart)\n(expressed g3 skin)\n(interacts g1 g2)\n(interacts g3 g1)\n(annotated g1 go7)\n"},
        {"d2", "(target rs1 g1)\n(target rs1 g1)\n(target rs1 g1)\n(target rs1 g2)\n(target rs2 g1)\n(expressed g1 liver)\n"
               "(expressed g1 liver)\n(expressed g2 heart)\n(expressed g2 lung)\n(interacts g2 g1)\n(interacts g2 g1)\n(interacts g1 g1)\n"},
        {"d3", "(target rs1 g1)\n(target rs1 g2)\n(expressed g2 heart)\n(expressed g3 liver)\n(expressed g1 g1)\n"
               "(interacts g1 g2)\n(interacts g2 g2)\n(target g1 g1)\n"},
        {"d4", "(target rs1 g1)\n(target rs1 gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx)\n"
               "(expressed g1 liver)\n(expressed g1 liver)\n(expressed gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx heart)\n"
               "(interacts gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx g1)\n"},
        {"d5", "(target rs1 g1)\n(target rs1 g2)\n(target rs1 g2)\n(expressed g1 tissue0)\n(expressed g1 tissue0)\n(expressed g1 liver)\n"
               "(expressed g2 tissue0)\n(interacts g2 g1)\n(interacts g1 g2)\n(target tissue0 g1)\n(expressed tissue0 tissue0)\n(target rs3 tissue0)\n"},
        /* d6: every row of a seed filtered, one row and several rows (CeTTa's Empty) */
        {"d6", "(target rs1 g1)\n(expressed g1 tissue0)\n(expressed g1 tissue0)\n(target rs2 g2)\n(target rs2 g3)\n"
               "(expressed g2 tissue0)\n(expressed g3 tissue0)\n(target rs4 g4)\n(expressed g4 tissue0)\n(target rs5 g5)\n(target rs5 g4)\n(expressed g5 kidney)\n"
               "(interacts g2 g1)\n(interacts g4 g4)\n"},
    };
    int fails = 0;
    for (size_t i = 0; i < sizeof spaces / sizeof *spaces; i++) {
        char *copy = strdup(spaces[i][1]), *seeds[512]; int ns = 0;
        for (char *s = strtok(copy, " ()\n"); s && ns < 510; s = strtok(NULL, " ()\n")) {
            bool dup = false;
            for (int j = 0; j < ns; j++) if (!strcmp(seeds[j], s)) dup = true;
            if (!dup) seeds[ns++] = s;
        }
        seeds[ns++] = "absent_seed_zz";
        difftest_one(spaces[i][0], spaces[i][1], seeds, ns, &fails);
        free(copy);
    }
    if (kbfile && qfile) {
        char *kb = cpk_read_file(kbfile, NULL), *qs = cpk_read_file(qfile, NULL);
        if (!kb || !qs) die("read inputs");
        char *seeds[4096]; int ns = 0;
        for (char *s = strtok(qs, "\n"); s && ns < 4096; s = strtok(NULL, "\n")) seeds[ns++] = s;
        difftest_one("kb", kb, seeds, ns, &fails);
        free(kb); free(qs);
    }
    printf("summary: %d failed\n", fails);
    return fails ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc < 2) die("usage: cettapack-probe bench|difftest|defs ...");
    const char *kb = NULL, *q = NULL, *mode = "generic";
    int repeat = 5;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--kb") && i + 1 < argc) kb = argv[++i];
        else if (!strcmp(argv[i], "--queries") && i + 1 < argc) q = argv[++i];
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) mode = argv[++i];
        else if (!strcmp(argv[i], "--repeat") && i + 1 < argc) repeat = atoi(argv[++i]);
        else die("unknown argument %s", argv[i]);
    }
    if (!strcmp(argv[1], "bench")) { if (!kb || !q) die("bench needs --kb and --queries"); return cmd_bench(kb, q, mode, repeat < 1 ? 1 : repeat); }
    if (!strcmp(argv[1], "difftest")) return cmd_difftest(kb, q);
    if (!strcmp(argv[1], "defs")) { char *d = fn_defs(); fputs(d, stdout); free(d); return 0; }
    die("unknown command %s", argv[1]);
    return 2;
}
